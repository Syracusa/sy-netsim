#include <errno.h>

#include "log.h"
#include "sim_hdlreport.h"
#include "mq.h"
#include "report_msg.h"
#include "netutil.h"

static void process_net_trx_report(int report_node,
                                   void *data,
                                   int len)
{
    NetTrxReport *report = (NetTrxReport *)data;
    TLOGD("NetTrxReport(%d->%s) : Tx %d  Rx %d\n", 
        report_node, ip2str(report->peer_addr),
        report->tx, report->rx);
}

static void handle_net_report(SimulatorCtx *sctx,
                              int report_node,
                              long type,
                              void *data,
                              int len)
{
    switch (type) {
        case REPORT_MSG_NET_TRX:
            process_net_trx_report(report_node, data, len);
            break;
        default:
            TLOGE("Unhandled net report %ld\n", type);
            break;
    }
}

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

static void recv_net_report(SimulatorCtx *sctx)
{
    MqMsgbuf msg;
    for (int i = 0; i < MAX_NODE_ID; i++) {
        ssize_t ret = msgrcv(sctx->nodes[i].mqid_net_report, &msg,
                             MQ_MAX_DATA_LEN, 0, IPC_NOWAIT);
        if (ret == 0) {
            break;
        } else if (ret > 0) {
            // TLOGD("Recv net report: type : %ld, length : %ld\n",
            //       msg.type, ret);
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

void recv_report(SimulatorCtx *sctx)
{
    recv_phy_report(sctx);
    recv_mac_report(sctx);
    recv_net_report(sctx);
}
