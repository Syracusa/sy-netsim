#include "olsr_forwarding.h"
#include "olsr.h"

static int is_mpr_selector(OlsrContext *ctx,
                           in_addr_t addr)
{
    MprSelectorElem *selem = (MprSelectorElem *)
        rbtree_search(ctx->selector_tree, &addr);
    if (selem)
        return 1;
    return 0;
}

typedef struct AddrListElem {
    CllElem elem;
    in_addr_t addr;
} AddrListElem;

static void destroy_addr_list(DuplicateSetElem *delem)
{
    CllElem *l = NULL;
    CllElem *prev = NULL;
    cll_foreach(l, &delem->iface_addr_list)
    {
        if (l->prev != &delem->iface_addr_list)
            free(l->prev);
    }

    if (l->prev != l)
        free(l->prev);
}

static void expire_dup_entry_cb(void *arg)
{
    OlsrContext *ctx = &g_olsr_ctx;
    DuplicateSetElem *delem = arg;

    rbtree_delete(ctx->dup_tree, &delem->key);
    destroy_addr_list(delem);
    timerqueue_free_timer(delem->expire_timer);

    free(delem);
}

static DuplicateSetElem *create_dup_entry(OlsrContext *ctx,
                                          DuplicateSetKey dkey)
{
    DuplicateSetElem *delem =
        (DuplicateSetElem *)malloc(sizeof(DuplicateSetElem));
    memset(delem, 0x00, sizeof(DuplicateSetElem));

    delem->rbn.key = &delem->key;
    delem->key = dkey;
    cll_init_head(&delem->iface_addr_list);

    AddrListElem *aelem = (AddrListElem *)
        malloc(sizeof(AddrListElem));

    aelem->addr = ctx->conf.own_ip;
    cll_add_tail(&delem->iface_addr_list, (CllElem *)aelem);

    delem->retransmitted = 0;

    TimerqueueElem *timer = timerqueue_new_timer();
    timer->callback = expire_dup_entry_cb;
    timer->arg = delem;
    timer->interval_us = DEF_DUP_HOLD_TIME_MS * 1000;
    timer->use_once = 1;
    timerqueue_register_timer(ctx->timerqueue, timer);

    delem->expire_timer = timer;
}

void olsr_msg_forwarding(OlsrContext *ctx,
                         OlsrMsgHeader *msg,
                         in_addr_t src)
{
    DuplicateSetKey dkey;
    dkey.orig = msg->originator;
    dkey.seq = ntohs(msg->seqno);

    DuplicateSetElem *delem = (DuplicateSetElem *)
        rbtree_search(ctx->dup_tree, &dkey);

    if (delem) {
        if (delem->retransmitted != 0)
            return;
    } else {
        delem = create_dup_entry(ctx, dkey);
    }

    if (!is_mpr_selector(ctx, src))
        return;

    msg->ttl -= 1;
    msg->hopcnt += 1;

    if (msg->ttl == 0)
        return;

    /* Finally forwarding msg */
    TLOGD("Forwarding msg from %s(orignator %s)\n",
          ip2str(src),
          ip2str(msg->originator));
    RingBuffer_push(ctx->olsr_tx_msgbuf, msg, ntohs(msg->olsr_msgsize));
}