#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>

#include "simphy.h"

#include "log.h"
#include "params.h"
#include "mq.h"
#include "util.h"

#include "config_msg.h"
#include "report_msg.h"

char dbgname[10];
FILE* dbgfile;

void init_mq(SimPhyCtx *spctx)
{
    for (int nid = 0; nid < MAX_NODE_ID; nid++) {
        int mqkey_send_mac = MQ_KEY_PHY_TO_MAC + nid;
        spctx->nodes[nid].mqid_send_mac = msgget(mqkey_send_mac, IPC_CREAT | 0666);
        mq_flush(spctx->nodes[nid].mqid_send_mac);

        int mqkey_recv_mac = MQ_KEY_MAC_TO_PHY + nid;
        spctx->nodes[nid].mqid_recv_mac = msgget(mqkey_recv_mac, IPC_CREAT | 0666);
        mq_flush(spctx->nodes[nid].mqid_recv_mac);

        if (DEBUG_MQ)
            TLOGI("Message Queue ID(Node %d) : MAC->PHY %d(%d)   PHY->MAC %d(%d)\n", nid,
                  spctx->nodes[nid].mqid_recv_mac, mqkey_recv_mac,
                  spctx->nodes[nid].mqid_send_mac, mqkey_send_mac);
    }

    int mqkey_send_report = MQ_KEY_PHY_REPORT;
    spctx->mqid_send_report = msgget(mqkey_send_report, IPC_CREAT | 0666);
    mq_flush(spctx->mqid_send_report);

    int mqkey_recv_command = MQ_KEY_PHY_COMMAND;
    spctx->mqid_recv_command = msgget(mqkey_recv_command, IPC_CREAT | 0666);
    mq_flush(spctx->mqid_recv_command);
}

static void send_to_remote_mac(SimPhyCtx *spctx,
                               int id,
                               void *data, size_t len)
{
    /* TODO : Multicast */
}

static void recv_from_remote_mac(SimPhyCtx *spctx,
                                 int id,
                                 void *data,
                                 size_t len)
{
    /* TODO : Multicast Recv */
}

static void send_to_local_mac(SimPhyCtx *spctx,
                              int receiver_nid,
                              void *data,
                              size_t len,
                              long type)
{
    /* The mtype field must have a strictly positive integer value. */
    if (type < 1) {
        TLOGE("Can't send meg with type %ld\n", type);
        return;
    }

    MqMsgbuf msg;
    msg.type = type;

    if (len > MQ_MAX_DATA_LEN) {
        TLOGE("Can't send data with length %lu\n", len);
    }
    memcpy(msg.text, data, len);
    int ret = msgsnd(spctx->nodes[receiver_nid].mqid_send_mac, &msg, len, IPC_NOWAIT);
    if (ret < 0) {
        if (errno == EAGAIN) {
            TLOGE("Message queue full!\n");
        } else {
            TLOGE("Can't send to mac. mqid: %d len: %lu(%s)\n",
                  spctx->nodes[receiver_nid].mqid_send_mac, len, strerror(errno));
        }
    }
}

static void process_mac_frame(SimPhyCtx *spctx,
                              int sender_nid,
                              void *data,
                              size_t len)
{
    for (int nid = 0; nid < MAX_NODE_ID; nid++) {
        if (sender_nid == nid)
            continue;

        if (spctx->nodes[nid].alive == 1) {
            if (DEBUG_MAC_TRX)
                TLOGI("PHY msg forwarding... %d -> %d\n", sender_nid, nid);

            SimLink *link = &spctx->links[sender_nid][nid];
            if (link->los != 1) {
                if (DEBUG_PHY_DROP)
                    TLOGW("PHY DROP FRAME %d => %d LOS\n", sender_nid, nid);
                continue;
            }
            send_to_local_mac(spctx, nid, data, len, MESSAGE_TYPE_DATA);
        }
    }
}

static void recv_from_local_mac(SimPhyCtx *spctx)
{
    for (int nid = 0; nid < MAX_NODE_ID; nid++) {
        MqMsgbuf msg;
        while (1) {
            ssize_t res = msgrcv(spctx->nodes[nid].mqid_recv_mac,
                                 &msg, sizeof(msg.text),
                                 0, IPC_NOWAIT);
            if (res < 0) {
                if (errno != ENOMSG) {
                    fprintf(stderr, "Msgrcv failed(err: %s)\n", strerror(errno));
                }
                break;
            }
            spctx->nodes[nid].alive = 1;

            switch (msg.type) {
                case MESSAGE_TYPE_DATA:
                    process_mac_frame(spctx, nid, msg.text, res);
                    break;
                case MESSAGE_TYPE_HEARTBEAT:
                    TLOGE("Heartbeat recvd from MAC %d\n", nid);
                    break;
                default:
                    TLOGE("Unknown msgtype %ld\n", msg.type);
                    break;
            }

        }
    }
}

static void handle_link_config_command(SimPhyCtx *spctx,
                                       void *msg)
{
    PhyLinkConfig *lmsg = msg;

    SimLink *l1 = &spctx->links[lmsg->node_id_1][lmsg->node_id_2];
    SimLink *l2 = &spctx->links[lmsg->node_id_2][lmsg->node_id_1];

    l1->los = l2->los = lmsg->los;
    l1->pathloss_x100 = l2->pathloss_x100 = lmsg->pathloss_x100;

    TLOGI("Link %u <=> %u  LOS %u  PASSLOSS %u\n",
          lmsg->node_id_1, lmsg->node_id_2,
          lmsg->los, lmsg->pathloss_x100);
}

static void recv_command(SimPhyCtx *spctx)
{
    MqMsgbuf msg;
    while (1) {
        ssize_t res = msgrcv(spctx->mqid_recv_command,
                             &msg, sizeof(msg.text),
                             0, IPC_NOWAIT);
        if (res < 0) {
            if (errno != ENOMSG) {
                fprintf(stderr, "Msgrcv failed(err: %s)\n", strerror(errno));
            }
            break;
        }

        switch (msg.type) {
            case CONF_MSG_TYPE_PHY_LINK_CONFIG:
                handle_link_config_command(spctx, msg.text);
                break;
            default:
                TLOGE("PHY Config Unknown msgtype %ld\n", msg.type);
                break;
        }

    }
}

static SimPhyCtx *create_context()
{
    SimPhyCtx *spctx = malloc(sizeof(SimPhyCtx));
    memset(spctx, 0x00, sizeof(SimPhyCtx));

    for (int i = 0; i < MAX_NODE_ID; i++) {
        for (int j = 0; j < MAX_NODE_ID; j++) {
            spctx->links[i][j].los = 1;
            spctx->links[i][j].pathloss_x100 = 0;
        }
    }

    return spctx;
}

static void delete_context(SimPhyCtx *spctx)
{
    free(spctx);
}

static void mainloop(SimPhyCtx *spctx)
{
    struct timespec before, after, diff, req, rem;

    while (1) {
        clock_gettime(CLOCK_REALTIME, &before);

        recv_from_local_mac(spctx);
        recv_command(spctx);

        clock_gettime(CLOCK_REALTIME, &after);
        timespec_sub(&after, &before, &diff);
        if (diff.tv_nsec < 1000 * 1000) {
            req.tv_sec = 0;
            req.tv_nsec = 1000 * 1000 - diff.tv_nsec;
            nanosleep(&req, &rem);
        }
    }
}

int main()
{
    sprintf(dbgname, "PHY   ");
    dbgfile = stderr;
    TLOGI("SIMPHY STARTED\n");
    SimPhyCtx *spctx = create_context();

    init_mq(spctx);
    mainloop(spctx);

    delete_context(spctx);
}