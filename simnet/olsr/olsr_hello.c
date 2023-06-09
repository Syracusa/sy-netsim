#include "olsr_hello.h"
#include "olsr.h"

static void dump_hello(void *data,
                       size_t msg_size,
                       const char *identifier)
{
    if (!DUMP_HELLO_MSG)
        return;

    char logbuf[1000];
    char *log_offset = logbuf;
    
    uint8_t *start = data;
    uint8_t *offset = data;

    HelloMsg *hello = (HelloMsg *)data;
    offset += sizeof(HelloMsg);
    log_offset += sprintf(log_offset, "\n");
    log_offset += sprintf(log_offset, " === Dump Hello(From %s) ===\n", identifier);
    log_offset += sprintf(log_offset, "> Willingness : %u\n", hello->willingness);
    log_offset += sprintf(log_offset, "> HTime : %u\n", me_to_reltime(hello->htime));

    while (msg_size > offset - start) {
        HelloInfo *info = (HelloInfo *)offset;
        offset += sizeof(info);

        int entrysz = ntohs(info->size);
        log_offset += sprintf(log_offset, "> Link code %s Neibor status %s EntryNum %d\n",
              link_status_str(EXTRACT_LINK(info->link_code)),
              neighbor_status_str(EXTRACT_STATUS(info->link_code)),
              entrysz);
        for (int i = 0; i < entrysz; i++) {
            log_offset += sprintf(log_offset, "> > %s\n", ip2str(info->neigh_addr[i]));
        }
        offset += sizeof(in_addr_t) * msg_size;
    }
    log_offset += sprintf(log_offset, "\n");
    TLOGD("%s", logbuf);
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

static void mpr_selector_expire(void *arg)
{
    OlsrContext *ctx = &g_olsr_ctx;
    MprSelectorElem *exp_elem = arg;

    TLOGD("Mpr selector expire %s\n", ip2str(exp_elem->selector_addr));

    rbtree_delete(ctx->selector_tree, &exp_elem->selector_addr);
    timerqueue_free_timer(exp_elem->expire_timer);
    free(exp_elem);
}

static void populate_linkset(OlsrContext *ctx,
                             in_addr_t orig,
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
        uint8_t neighstat = EXTRACT_STATUS(linkcode_to_me);

        MprSelectorElem *mpr_selector = NULL;
        switch (neighstat) {
            case MPR_NEIGH:
                /* Populate MPR Selector Set */
                mpr_selector = (MprSelectorElem *)
                    rbtree_search(ctx->selector_tree,
                                  &link->neighbor_iface_addr);
                if (!mpr_selector) {
                    mpr_selector = malloc(sizeof(MprSelectorElem));
                    mpr_selector->rbn.key = &mpr_selector->selector_addr;
                    mpr_selector->selector_addr = orig;
                    mpr_selector->expire_timer = timerqueue_new_timer();
                    mpr_selector->expire_timer->callback = mpr_selector_expire;
                    mpr_selector->expire_timer->arg = mpr_selector;
                    timerqueue_register_timer(ctx->timerqueue,
                                              mpr_selector->expire_timer);
                }
                mpr_selector->expire_timer->interval_us = vtime * 1000;
                timerqueue_reactivate_timer(ctx->timerqueue,
                                            mpr_selector->expire_timer);
                rbtree_insert(ctx->selector_tree, (rbnode_type *)mpr_selector);
                break;
            default:
                break;
        }
    } else {
        TLOGI("Neighbor doesn't have my information!\n");
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

static LocalNetIfaceElem *make_new_local_iface_elem(OlsrContext *ctx)
{
    LocalNetIfaceElem *iface;

    iface = malloc(sizeof(LocalNetIfaceElem));
    memset(iface, 0x00, sizeof(LocalNetIfaceElem));
    iface->rbn.key = &(iface->local_iface_addr);
    iface->local_iface_addr = ctx->conf.own_ip;
    iface->iface_link_tree = rbtree_create(rbtree_compare_by_inetaddr);
    rbtree_insert(ctx->local_iface_tree, (rbnode_type *)iface);
    TLOGD("New local interface %s registered\n",
          ip2str(iface->local_iface_addr));

    return iface;
}

void process_olsr_hello(OlsrContext *ctx,
                        in_addr_t src,
                        in_addr_t orig,
                        void *hello,
                        olsr_reltime vtime,
                        size_t msgsize)
{
    dump_hello(hello, msgsize, ip2str(src));
    HelloMsg *hello_msg = (HelloMsg *)hello;
    uint8_t willingness = hello_msg->willingness;
    (void)willingness;

    LocalNetIfaceElem *iface = (LocalNetIfaceElem *)
        rbtree_search(ctx->local_iface_tree, &ctx->conf.own_ip);

    if (iface == NULL)
        iface = make_new_local_iface_elem(ctx);

    NeighborElem *neigh =
        (NeighborElem *)rbtree_search(ctx->neighbor_tree, &src);
    if (neigh == NULL) {
        neigh = make_neighbor_elem(ctx, src, hello_msg->willingness);
        rbtree_insert(ctx->neighbor_tree, (rbnode_type *)neigh);
    }

    LinkElem *link = (LinkElem *)rbtree_search(iface->iface_link_tree, &src);
    if (link == NULL) {
        TLOGI("New link found on local iface %s to src %s\n",
              ip2str(iface->local_iface_addr), ip2str(src));
        link = make_link_elem(ctx, src, ctx->conf.own_ip, vtime);
        rbtree_insert(iface->iface_link_tree, (rbnode_type *)link);
    } else {
        link_elem_expire_timer_set(ctx, link, vtime);
    }

    link_elem_asym_timer_set(ctx, link, vtime);
    populate_linkset(ctx, orig, link, hello_msg->hello_info,
                     msgsize - sizeof(HelloMsg), vtime);

    update_neighbor_status(ctx, neigh);

    if (neigh->status == SYM_NEIGH ||
        neigh->status == MPR_NEIGH) {

        populate_neigh2set(ctx, neigh, hello_msg->hello_info,
                           msgsize - sizeof(HelloMsg), vtime);
    }

    populate_mpr_set(ctx);
}