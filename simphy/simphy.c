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

char dbgname[10];

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

static void process_mac_msg(SimPhyCtx *spctx,
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
            send_to_local_mac(spctx, nid, data, len, 1);
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
            process_mac_msg(spctx, nid, msg.text, res);
        }
    }
}

static SimPhyCtx *create_context()
{
    SimPhyCtx *spctx = malloc(sizeof(SimPhyCtx));
    memset(spctx, 0x00, sizeof(SimPhyCtx));

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
    TLOGI("SIMPHY STARTED\n");
    SimPhyCtx *spctx = create_context();

    sprintf(dbgname, "PHY   ");

    init_mq(spctx);
    mainloop(spctx);

    delete_context(spctx);
}