#include "olsr_hello.h"
#include "olsr.h"

static uint8_t get_neighbor_status(OlsrContext *ctx,
                                   in_addr_t addr)
{
    NeighborElem *elem;
    elem = (NeighborElem *)rbtree_search(ctx->neighbor_tree, &addr);

    if (elem != (NeighborElem *)RBTREE_NULL)
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
    *offsetp += sizeof(HelloInfo);

    RBTREE_FOR(ielem, LocalNetIfaceElem *, ctx->neighbor_tree)
    {
        LinkElem *lelem;
        RBTREE_FOR(lelem, LinkElem *, ielem->iface_link_tree)
        {
            uint8_t neighbor_status =
                get_neighbor_status(ctx, lelem->neighbor_iface_addr);

            if (neighbor_status == STATUS_UNAVAILABLE)
                continue;

            uint8_t link_code = CREATE_LINK_CODE(lelem->status, neighbor_status);

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
            uint8_t code = CREATE_LINK_CODE(lstat, nstat);
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
    hello_msg->htime = 0; /* TODO */
    hello_msg->willingness = ctx->param.willingness;

    offset += sizeof(HelloMsg);

    hello_write_links(ctx, &offset);
    *len = offset - start;
}

static void link_timer_expire(void *arg)
{
    LinkElem *link = arg;

    if (link->expire_timer->attached == 1) {
        link->expire_timer->active = 0;
        link->expire_timer->free_on_detach = 1;
    } else {
        free(link->expire_timer);
    }

    if (link->asym_timer->attached == 1) {
        link->asym_timer->active = 0;
        link->asym_timer->free_on_detach = 1;
    } else {
        free(link->asym_timer);
    }

    if (link->sym_timer->attached == 1) {
        link->sym_timer->active = 0;
        link->sym_timer->free_on_detach = 1;
    } else {
        free(link->sym_timer);
    }

    OlsrContext *ctx = &g_olsr_ctx;

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
    link->status = LOST_LINK;
}

static void sym_link_timer_expire(void *arg)
{
    LinkElem *link = arg;
    link->status = ASYM_LINK;
}

static void link_elem_expire_timer_set(OlsrContext *ctx,
                                       LinkElem *link)
{
    if (link->expire_timer == NULL) {
        link->expire_timer = timerqueue_new_timer();

        link->expire_timer->use_once = 1;
        link->expire_timer->callback = link_timer_expire;
        link->expire_timer->arg = link;
        link->expire_timer->interval_us = 8 * 1000 * 1000; /* TODO */
        timerqueue_register_timer(ctx->timerqueue, link->expire_timer);
    } else {
        timerqueue_reactivate_timer(ctx->timerqueue, link->expire_timer);
    }
}

static void link_elem_asym_timer_set(OlsrContext *ctx,
                                     LinkElem *link)
{
    if (link->status != SYM_LINK)
        link->status = ASYM_LINK;

    if (link->asym_timer == NULL) {
        link->asym_timer = timerqueue_new_timer();

        link->asym_timer->use_once = 1;
        link->asym_timer->callback = link_timer_expire;
        link->asym_timer->arg = link;
        link->asym_timer->interval_us = 6 * 1000 * 1000; /* TODO */
        timerqueue_register_timer(ctx->timerqueue, link->asym_timer);
    } else {
        timerqueue_reactivate_timer(ctx->timerqueue, link->asym_timer);
    }
}

static void link_elem_sym_timer_set(OlsrContext *ctx,
                                    LinkElem *link)
{
    link->status = SYM_LINK;
    if (link->sym_timer == NULL) {
        link->sym_timer = timerqueue_new_timer();

        link->sym_timer->use_once = 1;
        link->sym_timer->callback = link_timer_expire;
        link->sym_timer->arg = link;
        link->sym_timer->interval_us = 4 * 1000 * 1000; /* TODO */
        timerqueue_register_timer(ctx->timerqueue, link->sym_timer);
    } else {
        timerqueue_reactivate_timer(ctx->timerqueue, link->sym_timer);
    }
}

static LinkElem *make_link_elem(OlsrContext *ctx,
                                in_addr_t src,
                                in_addr_t my_addr)
{
    LinkElem *link = malloc(sizeof(LinkElem));
    memset(link, 0x00, sizeof(LinkElem));

    link->neighbor_iface_addr = src;
    link->local_iface_addr = my_addr;
    link->status = LOST_LINK;

    link_elem_expire_timer_set(ctx, link);
}

void process_olsr_hello(OlsrContext *ctx,
                        in_addr_t src, void *hello)
{
    HelloMsg *hello_msg = hello;

    LocalNetIfaceElem *iface =
        (LocalNetIfaceElem *)rbtree_search(ctx->local_iface_tree,
                                           &ctx->conf.own_ip);

    if (iface == (LocalNetIfaceElem *)RBTREE_NULL) {
        iface = malloc(sizeof(LocalNetIfaceElem));
        memset(iface, 0x00, sizeof(LocalNetIfaceElem));
        iface->priv_rbn.key = &(iface->local_iface_addr);
        iface->local_iface_addr = ctx->conf.own_ip;
        iface->iface_link_tree = rbtree_create(rbtree_compare_by_inetaddr);
        rbtree_insert(ctx->local_iface_tree, (rbnode_type *)iface);
    }

    LinkElem *link = (LinkElem *)rbtree_search(iface->iface_link_tree, &src);
    if (link == (LinkElem *)RBTREE_NULL) {
        link = make_link_elem(ctx, src, ctx->conf.own_ip);
    }
    link_elem_asym_timer_set(ctx, link);

    /* Parse Hello */

}