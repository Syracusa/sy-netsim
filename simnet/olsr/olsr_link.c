#include "olsr_link.h"
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

void link_timer_expire(void *arg)
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

void asym_link_timer_expire(void *arg)
{
    LinkElem *link = arg;
    if (LOG_LINK_TIMER)
        TLOGD("ASYM Link timer expired(To %s)\n",
              ip2str(link->neighbor_iface_addr));
    set_link_status(link, LOST_LINK);
}

void sym_link_timer_expire(void *arg)
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

void link_elem_expire_timer_set(OlsrContext *ctx,
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
        sprintf(link->expire_timer->debug_name, "link_timer_expire");
        timerqueue_register_timer(ctx->timerqueue, link->expire_timer);
    } else {
        link->expire_timer->interval_us = vtime * 1000;
        timerqueue_reactivate_timer(ctx->timerqueue, link->expire_timer);
    }
}

void link_elem_asym_timer_set(OlsrContext *ctx,
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
        sprintf(link->asym_timer->debug_name, "link_timer_expire");
        timerqueue_register_timer(ctx->timerqueue, link->asym_timer);
    } else {
        link->asym_timer->interval_us = vtime * 1000;
        timerqueue_reactivate_timer(ctx->timerqueue, link->asym_timer);
    }
}

void link_elem_sym_timer_set(OlsrContext *ctx,
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
        sprintf(link->sym_timer->debug_name, "link_timer_expire");
        timerqueue_register_timer(ctx->timerqueue, link->sym_timer);
    } else {
        link->sym_timer->interval_us = vtime * 1000;
        timerqueue_reactivate_timer(ctx->timerqueue, link->sym_timer);
    }
}

LinkElem *make_link_elem(OlsrContext *ctx,
                         in_addr_t src,
                         in_addr_t my_addr,
                         olsr_reltime vtime)
{
    TLOGI("Make new link elem. src : %s <=> me : %s\n",
          ip2str(src), ip2str(my_addr));
    LinkElem *link = malloc(sizeof(LinkElem));
    memset(link, 0x00, sizeof(LinkElem));

    link->rbn.key = &link->neighbor_iface_addr;
    link->neighbor_iface_addr = src;
    link->local_iface_addr = my_addr;
    set_link_status(link, LOST_LINK);

    link_elem_expire_timer_set(ctx, link, vtime);
    return link;
}
