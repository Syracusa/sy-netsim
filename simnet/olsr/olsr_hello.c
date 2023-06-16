#include "olsr_hello.h"
#include "olsr.h"

void set_link_status(LinkElem *link, uint8_t status)
{
    uint8_t old = link->status;
    if (old != status) {
        TLOGW("Link to %s  %s -> %s\n",
              ip2str(link->neighbor_iface_addr),
              link_status_str(old),
              link_status_str(status));
        link->status = status;
    }
}

void set_neighbor_status(NeighborElem *neighbor, uint8_t status)
{
    uint8_t old = neighbor->status;
    if (old != status) {
        TLOGW("Neighbor %s  Status %s -> %s\n",
              ip2str(neighbor->neighbor_main_addr),
              neighbor_status_str(old),
              neighbor_status_str(status));
        neighbor->status = status;
    }
}

static void dump_hello(void *data,
                       size_t msg_size,
                       const char *identifier)
{
    if (!DUMP_HELLO_MSG)
        return;

    uint8_t *start = data;
    uint8_t *offset = data;

    HelloMsg *hello = (HelloMsg *)data;
    offset += sizeof(HelloMsg);
    TLOGD("\n");
    TLOGD(" === Dump Hello(From %s) ===\n", identifier);
    TLOGD("> Willingness : %u\n", hello->willingness);
    TLOGD("> HTime : %u\n", me_to_reltime(hello->htime));

    while (msg_size > offset - start) {
        HelloInfo *info = (HelloInfo *)offset;
        offset += sizeof(info);

        int entrysz = ntohs(info->size);
        TLOGD("> Link code %s Neibor status %s EntryNum %d\n",
              link_status_str(EXTRACT_LINK(info->link_code)),
              neighbor_status_str(EXTRACT_STATUS(info->link_code)),
              entrysz);
        for (int i = 0; i < entrysz; i++) {
            TLOGD("> > %s\n", ip2str(info->neigh_addr[i]));
        }
        offset += sizeof(in_addr_t) * msg_size;
    }
    TLOGD("\n");
}

static uint8_t get_neighbor_status(OlsrContext *ctx,
                                   in_addr_t addr)
{
    NeighborElem *elem;
    elem = (NeighborElem *)rbtree_search(ctx->neighbor_tree, &addr);

    if (elem != NULL)
        return elem->status;

    return STATUS_UNAVAILABLE;
}

static void hello_write_links_with_code(OlsrContext *ctx,
                                        uint8_t **offsetp,
                                        uint8_t code)
{
    LocalNetIfaceElem *ielem;
    uint16_t write_count = 0;

    HelloInfo *hello_info = (HelloInfo *)*offsetp;
    hello_info->link_code = code;

    *offsetp += sizeof(HelloInfo);

    RBTREE_FOR(ielem, LocalNetIfaceElem *, ctx->local_iface_tree)
    {
        LinkElem *lelem;
        RBTREE_FOR(lelem, LinkElem *, ielem->iface_link_tree)
        {
            uint8_t neighbor_status =
                get_neighbor_status(ctx, lelem->neighbor_iface_addr);

            if (neighbor_status == STATUS_UNAVAILABLE) {
                TLOGD("Neighbor status not available!\n");
                continue;
            }

            uint8_t link_code = CREATE_LINK_CODE(neighbor_status, lelem->status);

            if (link_code == code) {
                hello_info->neigh_addr[write_count] = lelem->neighbor_iface_addr;
                write_count++;
                *offsetp += sizeof(lelem->neighbor_iface_addr);
            }
        }
    }

    if (write_count == 0) {
        *offsetp -= sizeof(HelloInfo);
    } else {
        hello_info->size = htons(write_count);
    }
}

static void hello_write_links(OlsrContext *ctx,
                              uint8_t **offset)
{
    for (uint8_t nstat = 0; nstat < NEIGHBOR_STATUS_END; nstat++) {
        for (uint8_t lstat = 0; lstat < LINK_STATUS_END; lstat++) {
            uint8_t code = CREATE_LINK_CODE(nstat, lstat);
            hello_write_links_with_code(ctx, offset, code);
        }
    }
}

void build_olsr_hello(OlsrContext *ctx,
                      void *buf,
                      size_t *len)
{
    uint8_t *start = buf;
    uint8_t *offset = buf;

    HelloMsg *hello_msg = (HelloMsg *)offset;
    hello_msg->reserved = 0;
    hello_msg->htime = reltime_to_me(DEF_HELLO_INTERVAL_MS);
    hello_msg->willingness = ctx->param.willingness;

    offset += sizeof(HelloMsg);

    hello_write_links(ctx, &offset);
    *len = offset - start;
    dump_hello(start, *len, "Own transmit");
}

static void link_timer_expire(void *arg)
{
    LinkElem *link = arg;
    if (LOG_LINK_TIMER)
        TLOGD("ASYM Link timer expired(To %s)\n",
              ip2str(link->neighbor_iface_addr));

    OlsrContext *ctx = &g_olsr_ctx;

    if (link->expire_timer)
        timerqueue_free_timer(link->expire_timer);

    if (link->asym_timer)
        timerqueue_free_timer(link->asym_timer);

    if (link->sym_timer)
        timerqueue_free_timer(link->sym_timer);

    LocalNetIfaceElem *iface;
    iface = (LocalNetIfaceElem *)rbtree_search(ctx->local_iface_tree,
                                               &link->local_iface_addr);

    /* Detach & Free itself */
    rbtree_delete(iface->iface_link_tree, &link->neighbor_iface_addr);
    free(link);
}

static void asym_link_timer_expire(void *arg)
{
    LinkElem *link = arg;
    if (LOG_LINK_TIMER)
        TLOGD("ASYM Link timer expired(To %s)\n",
              ip2str(link->neighbor_iface_addr));
    set_link_status(link, LOST_LINK);
}

static void sym_link_timer_expire(void *arg)
{
    LinkElem *link = arg;
    if (LOG_LINK_TIMER)
        TLOGD("SYM Link timer expired(To %s)\n",
              ip2str(link->neighbor_iface_addr));
    if (link->asym_timer->active == 1) {
        set_link_status(link, ASYM_LINK);
    } else {
        set_link_status(link, LOST_LINK);
    }
}

static void link_elem_expire_timer_set(OlsrContext *ctx,
                                       LinkElem *link,
                                       olsr_reltime vtime)
{
    if (LOG_LINK_TIMER)
        TLOGD("Link timer set\n");
    if (link->expire_timer == NULL) {
        link->expire_timer = timerqueue_new_timer();

        link->expire_timer->use_once = 1;
        link->expire_timer->callback = link_timer_expire;
        link->expire_timer->arg = link;
        link->expire_timer->interval_us = vtime * 1000;
        timerqueue_register_timer(ctx->timerqueue, link->expire_timer);
    } else {
        link->expire_timer->interval_us = vtime * 1000;
        timerqueue_reactivate_timer(ctx->timerqueue, link->expire_timer);
    }
}

static void link_elem_asym_timer_set(OlsrContext *ctx,
                                     LinkElem *link,
                                     olsr_reltime vtime)
{
    if (LOG_LINK_TIMER)
        TLOGD("ASYM link timer set\n");
    if (link->status != SYM_LINK) {
        set_link_status(link, ASYM_LINK);
    }

    if (link->asym_timer == NULL) {
        link->asym_timer = timerqueue_new_timer();

        link->asym_timer->use_once = 1;
        link->asym_timer->callback = link_timer_expire;
        link->asym_timer->arg = link;
        link->asym_timer->interval_us = vtime * 1000;
        timerqueue_register_timer(ctx->timerqueue, link->asym_timer);
    } else {
        link->asym_timer->interval_us = vtime * 1000;
        timerqueue_reactivate_timer(ctx->timerqueue, link->asym_timer);
    }
}

static void link_elem_sym_timer_set(OlsrContext *ctx,
                                    LinkElem *link,
                                    olsr_reltime vtime)
{
    if (LOG_LINK_TIMER)
        TLOGD("SYM link timer set\n");
    set_link_status(link, SYM_LINK);

    if (link->sym_timer == NULL) {
        link->sym_timer = timerqueue_new_timer();

        link->sym_timer->use_once = 1;
        link->sym_timer->callback = link_timer_expire;
        link->sym_timer->arg = link;
        link->sym_timer->interval_us = vtime * 1000;
        timerqueue_register_timer(ctx->timerqueue, link->sym_timer);
    } else {
        link->sym_timer->interval_us = vtime * 1000;
        timerqueue_reactivate_timer(ctx->timerqueue, link->sym_timer);
    }
}

static NeighborElem *make_neighbor_elem(OlsrContext *ctx,
                                        in_addr_t neigh_addr,
                                        uint8_t willingness)
{
    NeighborElem *neigh = malloc(sizeof(NeighborElem));

    neigh->priv_rbn.key = &neigh->neighbor_main_addr;
    neigh->neighbor_main_addr = neigh_addr;
    neigh->status = STATUS_UNAVAILABLE;
    neigh->willingness = willingness;
    neigh->neighbor2_tree = rbtree_create(rbtree_compare_by_inetaddr);

    return neigh;
}

static LinkElem *make_link_elem(OlsrContext *ctx,
                                in_addr_t src,
                                in_addr_t my_addr,
                                olsr_reltime vtime)
{
    TLOGI("Make new link elem. src : %s <=> me : %s\n",
          ip2str(src), ip2str(my_addr));
    LinkElem *link = malloc(sizeof(LinkElem));
    memset(link, 0x00, sizeof(LinkElem));

    link->priv_rbn.key = &link->neighbor_iface_addr;
    link->neighbor_iface_addr = src;
    link->local_iface_addr = my_addr;
    set_link_status(link, LOST_LINK);

    link_elem_expire_timer_set(ctx, link, vtime);
    return link;
}

static void populate_linkset(OlsrContext *ctx,
                             LinkElem *link,
                             void *hello_info,
                             size_t len,
                             olsr_reltime vtime)
{
    uint8_t *start = hello_info;
    uint8_t *offset = hello_info;

    uint8_t linkcode_to_me = STATUS_UNAVAILABLE;

    while (len > offset - start) {
        HelloInfo *hello_info = (HelloInfo *)offset;
        offset += sizeof(HelloInfo);

        int entrynum = ntohs(hello_info->size);
        for (int i = 0; i < entrynum; i++) {
            in_addr_t neigh_addr = hello_info->neigh_addr[i];
            if (LOG_HELLO_MSG)
                TLOGD("Hello from %s link state %u\n", ip2str(neigh_addr),
                      EXTRACT_LINK(hello_info->link_code));
            if (neigh_addr == ctx->conf.own_ip) {
                linkcode_to_me = hello_info->link_code;
                break;
            }
        }
        if (linkcode_to_me != STATUS_UNAVAILABLE)
            break;
        offset += sizeof(in_addr_t) * entrynum;
    }

    if (linkcode_to_me != STATUS_UNAVAILABLE) {
        uint8_t linkstat = EXTRACT_LINK(linkcode_to_me);
        switch (linkstat) {
            case SYM_LINK: /* Fallthrough */
            case ASYM_LINK:
                link_elem_sym_timer_set(ctx, link, vtime);
                break;
            case LOST_LINK:
                link->sym_timer->active = 0;
                sym_link_timer_expire(link);
                break;
            default:
                TLOGD("Unknown linkstat %u\n", linkstat);
                break;
        }
    } else {
        TLOGI("Neighbor doesn't have my information!\n");
    }
}

static void neigh2_elem_expire(void *arg)
{
    OlsrContext *ctx = &g_olsr_ctx;
    Neighbor2Elem *n2elem = arg;
    NeighborElem *neigh =
        (NeighborElem *)rbtree_search(ctx->neighbor_tree, &n2elem->bridge_addr);

    TLOGW("2-hop Neighbor %s bridged by %s Expired\n"
          , ip2str(n2elem->neighbor2_main_addr)
          , ip2str(neigh->neighbor_main_addr));

    timerqueue_free_timer(n2elem->expire_timer);
    rbtree_delete(neigh->neighbor2_tree, &n2elem->neighbor2_main_addr);
    free(n2elem);
}

static void update_neigh2_tuple(OlsrContext *ctx,
                                NeighborElem *neigh,
                                in_addr_t n2addr,
                                olsr_reltime vtime)
{
    Neighbor2Elem *n2elem =
        (Neighbor2Elem *)rbtree_search(neigh->neighbor2_tree, &n2addr);

    if (n2elem == NULL) {
        Neighbor2Elem *n2elem = (Neighbor2Elem *)malloc(sizeof(Neighbor2Elem));
        n2elem->priv_rbn.key = &n2elem->neighbor2_main_addr;
        n2elem->neighbor2_main_addr = n2addr;
        n2elem->bridge_addr = neigh->neighbor_main_addr;

        n2elem->expire_timer = timerqueue_new_timer();
        n2elem->expire_timer->interval_us = vtime * 1000;
        n2elem->expire_timer->use_once = 1;
        n2elem->expire_timer->arg = n2elem;
        n2elem->expire_timer->callback = neigh2_elem_expire;
        timerqueue_register_timer(ctx->timerqueue, n2elem->expire_timer);

        rbtree_insert(neigh->neighbor2_tree, (rbnode_type *)n2elem);
    } else {
        n2elem->expire_timer->interval_us = vtime * 1000;
        timerqueue_reactivate_timer(ctx->timerqueue, n2elem->expire_timer);
    }
}

static void populate_neigh2set(OlsrContext *ctx,
                               NeighborElem *neigh,
                               void *hello_info,
                               size_t len,
                               olsr_reltime vtime)
{
    uint8_t *start = hello_info;
    uint8_t *offset = hello_info;

    while (len > offset - start) {
        HelloInfo *hello_info = (HelloInfo *)offset;
        offset += sizeof(HelloInfo);

        int entrynum = ntohs(hello_info->size);
        for (int i = 0; i < entrynum; i++) {
            in_addr_t neigh2_addr = hello_info->neigh_addr[i];
            if (neigh2_addr == ctx->conf.own_ip)
                continue;
            update_neigh2_tuple(ctx, neigh, neigh2_addr, vtime);
        }
        offset += sizeof(in_addr_t) * entrynum;
    }
}

void update_neighbor_status(OlsrContext *ctx,
                            NeighborElem *neigh)
{
    int neighbor_is_sym = 0;
    LocalNetIfaceElem *ielem;
    RBTREE_FOR(ielem, LocalNetIfaceElem *, ctx->local_iface_tree)
    {
        LinkElem *lelem;
        RBTREE_FOR(lelem, LinkElem *, ielem->iface_link_tree)
        {
            if (lelem->neighbor_iface_addr == neigh->neighbor_main_addr) {
                if (lelem->status == SYM_LINK) {
                    neighbor_is_sym = 1;
                }
            }
        }
    }

    if (neighbor_is_sym) {
        set_neighbor_status(neigh, SYM_NEIGH);
    } else {
        set_neighbor_status(neigh, NOT_NEIGH);
    }
}

void process_olsr_hello(OlsrContext *ctx,
                        in_addr_t src,
                        void *hello,
                        olsr_reltime vtime,
                        size_t msgsize)
{
    LocalNetIfaceElem *iface =
        (LocalNetIfaceElem *)rbtree_search(ctx->local_iface_tree,
                                           &ctx->conf.own_ip);
    dump_hello(hello, msgsize, ip2str(src));
    HelloMsg *hello_msg = (HelloMsg *)hello;
    uint8_t willingness = hello_msg->willingness;

    if (iface == NULL) {
        iface = malloc(sizeof(LocalNetIfaceElem));
        memset(iface, 0x00, sizeof(LocalNetIfaceElem));
        iface->priv_rbn.key = &(iface->local_iface_addr);
        iface->local_iface_addr = ctx->conf.own_ip;
        iface->iface_link_tree = rbtree_create(rbtree_compare_by_inetaddr);
        rbtree_insert(ctx->local_iface_tree, (rbnode_type *)iface);
        TLOGD("New local interface %s registered\n",
              ip2str(iface->local_iface_addr));
    }

    NeighborElem *neigh =
        (NeighborElem *)rbtree_search(ctx->neighbor_tree, &src);
    if (neigh == NULL) {
        neigh = make_neighbor_elem(ctx, src, hello_msg->willingness);
        rbtree_insert(ctx->neighbor_tree, (rbnode_type *)neigh);
    }

    LinkElem *link = (LinkElem *)rbtree_search(iface->iface_link_tree, &src);
    if (link == NULL) {
        TLOGI("No link found on local iface %s to src %s\n",
              ip2str(iface->local_iface_addr), ip2str(src));
        link = make_link_elem(ctx, src, ctx->conf.own_ip, vtime);
        rbtree_insert(iface->iface_link_tree, (rbnode_type *)link);
    } else {
        link_elem_expire_timer_set(ctx, link, vtime);
    }

    link_elem_asym_timer_set(ctx, link, vtime);
    populate_linkset(ctx, link, hello_msg->hello_info,
                     msgsize - sizeof(HelloMsg), vtime);

    update_neighbor_status(ctx, neigh);

    if (neigh->status == SYM_NEIGH ||
        neigh->status == MPR_NEIGH) {

        populate_neigh2set(ctx, neigh, hello_msg->hello_info,
                           msgsize - sizeof(HelloMsg), vtime);
    }
}