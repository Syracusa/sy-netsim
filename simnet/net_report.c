#include "net_report.h"


static void send_report_to_simulator(SimNetCtx *snctx,
                                     int mqid,
                                     void *data,
                                     size_t len,
                                     long type)
{
    MqMsgbuf msg;
    msg.type = type;

    if (len > MQ_MAX_DATA_LEN) {
        TLOGE("Can't send data with length %lu\n", len);
    }
    memcpy(msg.text, data, len);
    int ret = msgsnd(mqid, &msg, len, IPC_NOWAIT);
    if (ret < 0) {
        if (errno == EAGAIN) {
            TLOGE("Message queue full!\n");
        } else {
            TLOGE("Can't send report. mqid: %d len: %lu(%s)\n",
                  mqid, len, strerror(errno));
        }
    }
}

static void send_route_report(SimNetCtx *snctx,
                              NeighborInfo *info,
                              in_addr_t neighbor_addr)
{
    char msgbuf[300];
    RoutingInfo* rinfo = &info->routing;

    NetRoutingReport *report = (NetRoutingReport *)msgbuf;
    report->target = ((uint8_t *)(&neighbor_addr))[2];
    report->visit_num = rinfo->hop_count;
    memcpy(report->path, rinfo->path, sizeof(in_addr_t) * rinfo->hop_count);

    send_report_to_simulator(
        snctx, snctx->mqid_send_report, report,
        sizeof(*report) + sizeof(in_addr_t) * rinfo->hop_count,
        REPORT_MSG_NET_ROUTING);
}

static void send_traffic_report(SimNetCtx *snctx,
                                NeighborInfo *info,
                                in_addr_t neighbor_addr)
{
    TrafficCountInfo *traffic = &info->traffic;

    NetTrxReport report;
    report.peer_addr = neighbor_addr;
    report.tx = traffic->tx_bytes;
    report.rx = traffic->rx_bytes;
    send_report_to_simulator(snctx, snctx->mqid_send_report,
                             &report, sizeof(report),
                             REPORT_MSG_NET_TRX);
}

void send_net_report_cb(void *arg)
{
    SimNetCtx *snctx = arg;
    NetStats *stat = &snctx->stat;

    for (int i = 0; i < stat->node_stats_num; i++) {
        NeighborInfo *info = &stat->node_info[i];
        in_addr_t neighbor_addr = info->addr;
        if (info->traffic.dirty) {
            send_traffic_report(snctx, info, neighbor_addr);
            info->traffic.dirty = 0;
        }

        if (info->routing.dirty) {
            TLOGD("Send routing report to %s\n", ip2str(neighbor_addr));
            send_route_report(snctx, info, neighbor_addr);
            info->routing.dirty = 0;
        }
    }
}

void start_report_job(SimNetCtx *snctx)
{
    TimerqueueElem *elem = timerqueue_new_timer();
    elem->callback = send_net_report_cb;
    elem->arg = snctx;
    elem->interval_us = 1000 * 100;
    timerqueue_register_timer(snctx->timerqueue, elem);
}