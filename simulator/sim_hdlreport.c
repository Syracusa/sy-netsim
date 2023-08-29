#include <errno.h>

#include "log.h"
#include "sim_hdlreport.h"
#include "mq.h"
#include "report_msg.h"
#include "netutil.h"

#define REPORT_MSG_VERBOSE 0

static void send_json_to_front(SimulatorCtx *sctx, char *json)
{
    /* Push to tcp send queue(json len, 2byte) */
    uint16_t jsonlen = htons(strlen(json));
    RingBuffer_push(sctx->server_ctx.sendq,
                    &jsonlen, 2);

    /* Push to tcp send queue(json itself) */
    RingBuffer_push(sctx->server_ctx.sendq,
                    json, strlen(json));
}

/** Recv trx report from simnet ==> Send to front */
static void process_net_trx_report(SimulatorCtx *sctx,
                                   int report_node,
                                   void *data,
                                   int len)
{
    NetTrxReport *report = (NetTrxReport *)data;
    if (REPORT_MSG_VERBOSE)
        TLOGD("NetTrxReport(%d->%s) : Tx %d  Rx %d\n",
              report_node, ip2str(report->peer_addr),
              report->tx, report->rx);

    /* Convert to JSON */
    char trx_report_json_buf[1024];
    sprintf(trx_report_json_buf,
           "{\"type\": \"TRx\", \"node\": %d, "
           "\"peer\": %u,"
           "\"tx\": %d, \"rx\": %d}",
           report_node,
           ((uint8_t *)(&report->peer_addr))[3],
           report->tx,
           report->rx);

    send_json_to_front(sctx, trx_report_json_buf);
}

/** Recv route report from simnet ==> Send to front */
static void process_net_route_report(SimulatorCtx *sctx,
                                     int report_node,
                                     void *data,
                                     int len)
{
    NetRoutingReport *report = (NetRoutingReport *)data;

    char route_report_json_buf[1024];
    char *offset = route_report_json_buf;

    offset += sprintf(offset,
                      "{\"type\": \"Route\", \"node\": %d, \"target\": %d,"
                      "\"hopcount\": %d, \"path\" : [",
                      report_node, report->target, report->visit_num);

    for (int i = 0; i < report->visit_num; i++) {
        offset += sprintf(offset, "%u", ((uint8_t *)(&report->path[i]))[2]);
        if (i != report->visit_num - 1)
            offset += sprintf(offset, ", ");
    }

    offset += sprintf(offset, "]}");

    if (REPORT_MSG_VERBOSE)
        TLOGI("route_report_json_buf : %s\n", route_report_json_buf);

    send_json_to_front(sctx, route_report_json_buf);
}

/** Parse simnet report msg type and connect to appropriate handler */
static void handle_net_report(SimulatorCtx *sctx,
                              int report_node,
                              long type,
                              void *data,
                              int len)
{
    switch (type) {
        case REPORT_MSG_NET_TRX:
            process_net_trx_report(sctx, report_node, data, len);
            break;
        case REPORT_MSG_NET_ROUTING:
            process_net_route_report(sctx, report_node, data, len);
            break;
        default:
            TLOGE("Unhandled net report %ld\n", type);
            break;
    }
}

/** Handle simphy report but currently no msg defined */
static void recv_phy_report(SimulatorCtx *sctx)
{
    MqMsgbuf msg;
    while (1) {
        ssize_t ret = msgrcv(sctx->mqid_phy_report, &msg,
                             MQ_MAX_DATA_LEN, 0, IPC_NOWAIT);
        if (ret == 0) {
            break;
        } else if (ret > 0) {
            TLOGD("Recv phy report: type : %ld, length : %ld\n",
                  msg.type, ret);
        } else {
            if (errno != ENOMSG) {
                TLOGF("Can't receive message from phy mqid: %d(%s)\n",
                      sctx->mqid_phy_report, strerror(errno));
            }
            break;
        }
    }
}

/** Handle simmac report but currently no msg defined */
static void recv_mac_report(SimulatorCtx *sctx)
{
    MqMsgbuf msg;

    for (int i = 0; i < MAX_NODE_ID; i++) {
        while (1) {
            ssize_t ret = msgrcv(sctx->nodes[i].mqid_mac_report, &msg,
                                 MQ_MAX_DATA_LEN, 0, IPC_NOWAIT);
            if (ret == 0) {
                break;
            } else if (ret > 0) {
                TLOGD("Recv mac report: type : %ld, length : %ld\n",
                      msg.type, ret);
            } else {
                if (errno != ENOMSG) {
                    TLOGF("Can't receive message from mac mqid: %d(%s)\n",
                          sctx->nodes[i].mqid_mac_report, strerror(errno));
                }
                break;
            }
        }
    }
}

/** Receive simnet report msg form message queue and call handler */
static void recv_net_report(SimulatorCtx *sctx)
{
    MqMsgbuf msg;
    for (int i = 0; i < MAX_NODE_ID; i++) {
        ssize_t ret = msgrcv(sctx->nodes[i].mqid_net_report, &msg,
                             MQ_MAX_DATA_LEN, 0, IPC_NOWAIT);
        if (ret == 0) {
            break;
        } else if (ret > 0) {
            if (REPORT_MSG_VERBOSE)
                TLOGD("Recv net report: type : %ld, length : %ld\n",
                      msg.type, ret);
            handle_net_report(sctx, i, msg.type, msg.text, ret);
        } else {
            if (errno != ENOMSG) {
                TLOGF("Can't receive message from net mqid: %d(%s)\n",
                      sctx->nodes[i].mqid_net_report, strerror(errno));
            }
            break;
        }
    }
}

/* Receive any kind of report msg from message queues */
void recv_report(SimulatorCtx *sctx)
{
    recv_phy_report(sctx);
    recv_mac_report(sctx);
    recv_net_report(sctx);
}
