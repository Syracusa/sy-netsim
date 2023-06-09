#include "olsr_tc.h"
#include "olsr.h"

static void dump_olsr_tc(TcMsg *tcmsg, int entrynum, const char *from)
{
    if (!DUMP_TC_MSG)
        return;

    char logbuf[1000];
    char *log_offset = logbuf;

    log_offset += sprintf(log_offset, "\n");
    log_offset += sprintf(log_offset, " === Dump TC (From %s) ===\n", from);
    log_offset += sprintf(log_offset, "> ANSN : %u\n", ntohs(tcmsg->ansn));
    if (entrynum != 0) {
        log_offset += sprintf(log_offset, "> Advertised Neighbors\n");
        for (int i = 0; i < entrynum; i++)
            log_offset += sprintf(log_offset, "> > %s\n", ip2str(tcmsg->neigh[i]));
    } else {
        log_offset += sprintf(log_offset, "> No Advertised Neighbor ");
    }
    log_offset += sprintf(log_offset, "\n");

    TLOGD("%s", logbuf);
}

void build_olsr_tc(OlsrContext *ctx, void *buf, size_t *len)
{
    uint8_t *offset = buf;
    uint8_t *start = buf;

    TcMsg *tc_msg = (TcMsg *)buf;
    offset += sizeof(TcMsg);

    tc_msg->ansn = htons(ctx->ansn);
    ctx->ansn++;

    int neighidx = 0;
    MprSelectorElem *sel_elem;
    RBTREE_FOR(sel_elem, MprSelectorElem *, ctx->selector_tree)
    {
        tc_msg->neigh[neighidx] = sel_elem->selector_addr;
        neighidx++;
        offset += sizeof(in_addr_t);
    }

    *len = offset - start;
    dump_olsr_tc(tc_msg, neighidx, "Own transmit");
}

void topology_info_timer_expire_cb(void *arg)
{
    OlsrContext *ctx = &g_olsr_ctx;
    TopologyInfoElem *telem = arg;

    rbtree_delete(ctx->topology_tree, &telem->dest_addr);
    traverse_postorder(telem->an_tree, free_arg, NULL);
    free(telem->an_tree);
    timerqueue_free_timer(telem->expire_timer);
    free(telem);

    calc_routing_table(ctx);
}

static int remove_obsolute_advertised_neighbor(TopologyInfoElem *telem)
{
    const int max_obsolutes = 10;
    in_addr_t obsolutes[max_obsolutes];
    int obsolutes_num = 0;

    AdvertisedNeighElem *aelem;
    RBTREE_FOR(aelem, AdvertisedNeighElem *, telem->an_tree)
    {
        if (aelem->obsolete) {
            obsolutes[obsolutes_num] = aelem->last_addr;
            obsolutes_num++;
        }
        if (obsolutes_num == max_obsolutes)
            break;
    }

    for (int i = 0; i < obsolutes_num; i++) {
        if (!rbtree_delete(telem->an_tree, &obsolutes[i])){
            TLOGE("Failed to delete obsolete advertised neighbor %s from %s\n",
                  ip2str(obsolutes[i]),
                  ip2str(telem->dest_addr));
        }
    }

    return obsolutes_num;
}

void process_olsr_tc(OlsrContext *ctx,
                     TcMsg *msg,
                     uint16_t tc_size,
                     in_addr_t src,
                     in_addr_t orig,
                     olsr_reltime vtime)
{
    NeighborElem *neigh = (NeighborElem *)
        rbtree_search(ctx->neighbor_tree, &src);

    if (!neigh)
        return;

    if (neigh->status != SYM_NEIGH && neigh->status != MPR_NEIGH)
        return;

    TopologyInfoElem *telem = (TopologyInfoElem *)
        rbtree_search(ctx->topology_tree, &orig);

    uint16_t ansn = ntohs(msg->ansn);
    if (!telem) {
        telem = (TopologyInfoElem *)malloc(sizeof(TopologyInfoElem));
        telem->rbn.key = &telem->dest_addr;
        telem->dest_addr = orig;
        telem->an_tree = rbtree_create(rbtree_compare_by_inetaddr);
        telem->seq = ansn;

        TimerqueueElem *timer = timerqueue_new_timer();
        timer->callback = topology_info_timer_expire_cb;
        timer->interval_us = vtime * 1000;
        timer->use_once = 1;
        timer->arg = telem;
        sprintf(timer->debug_name, "topology_info_timer_expire_cb");
        timerqueue_register_timer(ctx->timerqueue, timer);
        telem->expire_timer = timer;

        rbtree_insert(ctx->topology_tree, (rbnode_type *)telem);
    }

    if (ansn < telem->seq) {
        TLOGD("Ignore TC from %s (old seqno)", ip2str(orig));
        return;
    }

    telem->seq = ansn;
    timerqueue_reactivate_timer(ctx->timerqueue, telem->expire_timer);

    /* Mark all element in tree as obsolete */
    AdvertisedNeighElem *anelem;
    RBTREE_FOR(anelem, AdvertisedNeighElem *, telem->an_tree)
    {
        anelem->obsolete = 1;
    }

    int info_changed = 0;

    int entry_num = tc_size / sizeof(in_addr_t) - 1;
    dump_olsr_tc(msg, entry_num, ip2str(orig));
    for (int i = 0; i < entry_num; i++) {
        /* Advertised neighbor address */
        in_addr_t ana = msg->neigh[i];

        AdvertisedNeighElem *anelem = (AdvertisedNeighElem *)
            malloc(sizeof(AdvertisedNeighElem));
        anelem->rbn.key = &anelem->last_addr;
        anelem->last_addr = ana;
        anelem->obsolete = 0;
        anelem->duplicated = 0;

        if (rbtree_insert(telem->an_tree, (rbnode_type *)anelem)) {
            /* This mean new neighbor is added to TC */
            info_changed = 1;
            TLOGD("New advertised neighbor %s from %s\n",
                  ip2str(ana), ip2str(telem->dest_addr));
        } else {
            /* Neighbor keep exist in TC. Just unmark obsolete flag */
            free(anelem);
            anelem = (AdvertisedNeighElem *)
                rbtree_search(telem->an_tree, &ana);
            anelem->obsolete = 0;
        }
    }

    int obsoluted = remove_obsolute_advertised_neighbor(telem);
    if (obsoluted) {
        TLOGD("Obsoluted %d advertised neighbor(s) removed\n", obsoluted);
        info_changed = 1;
    }
    /* Check for topology change,
    If any change detected, Recalculate routing table */
    if (info_changed)
        calc_routing_table(ctx);
}