#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <errno.h>

#include "mq.h"
#include "util.h"

#include "log.h"
#include "params.h"

char dbgname[10];
FILE* dbgfile;

typedef struct
{
    int node_id;

    int mqid_recv_net;
    int mqid_send_net;
    int mqid_recv_phy;
    int mqid_send_phy;
} SimMacCtx;

void init_mq(SimMacCtx *smctx)
{
    int mqkey_send_net = MQ_KEY_MAC_TO_NET + smctx->node_id;
    smctx->mqid_send_net = msgget(mqkey_send_net, IPC_CREAT | 0666);

    int mqkey_recv_net = MQ_KEY_NET_TO_MAC + smctx->node_id;
    smctx->mqid_recv_net = msgget(mqkey_recv_net, IPC_CREAT | 0666);

    int mqkey_send_phy = MQ_KEY_MAC_TO_PHY + smctx->node_id;
    smctx->mqid_send_phy = msgget(mqkey_send_phy, IPC_CREAT | 0666);

    int mqkey_recv_phy = MQ_KEY_PHY_TO_MAC + smctx->node_id;
    smctx->mqid_recv_phy = msgget(mqkey_recv_phy, IPC_CREAT | 0666);

    if (DEBUG_MQ) {
        TLOGI("Message Queue ID : NET -> %d(%d) MAC %d(%d) -> PHY\n",
              smctx->mqid_recv_net, mqkey_recv_net,
              smctx->mqid_send_phy, mqkey_send_phy);
        TLOGI("Message Queue ID : NET <- %d(%d) MAC %d(%d) <- PHY\n",
              smctx->mqid_send_net, mqkey_send_net,
              smctx->mqid_recv_phy, mqkey_recv_phy);
    }

    mq_flush(smctx->mqid_send_net);
    mq_flush(smctx->mqid_recv_net);
    mq_flush(smctx->mqid_send_phy);
    mq_flush(smctx->mqid_recv_phy);
}

void sendto_net(SimMacCtx *smctx, void *data, size_t len, long type)
{
    send_mq(smctx->mqid_send_net, data, len, type);
}

void sendto_phy(SimMacCtx *smctx, void *data, size_t len, long type)
{
    send_mq(smctx->mqid_send_phy, data, len, type);
}

void process_net_msg(SimMacCtx *smctx, void *data, int len)
{
    if (DEBUG_MAC_TRX)
        TLOGI("Packet received from net. len : %d\n", len);
    sendto_phy(smctx, data, len, MESSAGE_TYPE_DATA);
}

void process_phy_msg(SimMacCtx *smctx, void *data, int len)
{
    if (DEBUG_MAC_TRX)
        TLOGI("Packet received from phy. len : %d\n", len);
    sendto_net(smctx, data, len, MESSAGE_TYPE_DATA);
}

static void poll_mq(SimMacCtx *smctx)
{
    MqMsgbuf msg;

    /* Receive from net */
    while (1) {
        ssize_t res = recv_mq(smctx->mqid_recv_net, &msg);
        if (res < 0) 
            break;
        
        process_net_msg(smctx, msg.text, res);
    }

    /* Receive from phy */
    while (1) {
        ssize_t res = recv_mq(smctx->mqid_recv_phy, &msg);
        if (res < 0)
            break;
            
        process_phy_msg(smctx, msg.text, res);
    }
}

void mainloop(SimMacCtx *smctx)
{
    struct timespec before, after, diff, req, rem;

    while (1) {
        clock_gettime(CLOCK_REALTIME, &before);

        poll_mq(smctx);

        clock_gettime(CLOCK_REALTIME, &after);
        timespec_sub(&after, &before, &diff);
        if (diff.tv_nsec < 1000 * 1000) {
            req.tv_sec = 0;
            req.tv_nsec = 1000 * 1000 - diff.tv_nsec;
            nanosleep(&req, &rem);
        }
    }
}

static SimMacCtx *create_simmac_context()
{
    SimMacCtx *smctx = malloc(sizeof(SimMacCtx));
    memset(smctx, 0x00, sizeof(SimMacCtx));
    return smctx;
}

static void delete_simmac_context(SimMacCtx *smctx)
{
    free(smctx);
}

static void parse_arg(SimMacCtx *smctx, int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage : ./%s {NODE_ID}\n", argv[0]);
        exit(2);
    }

    int res = sscanf(argv[1], "%u", &(smctx->node_id));

    if (res < 1) {
        fprintf(stderr, "Can't parse nodeID from [%s]\n", argv[1]);
        exit(2);
    }
}

int main(int argc, char *argv[])
{
    SimMacCtx *smctx = create_simmac_context();
    sprintf(dbgname, "MAC-%-2d", smctx->node_id);
    dbgfile = stderr;

    parse_arg(smctx, argc, argv);
    printf("Simnet start with nodeid %d\n", smctx->node_id);


    init_mq(smctx);

    sendto_phy(smctx, NULL, 0, MESSAGE_TYPE_HEARTBEAT);
    mainloop(smctx);

    delete_simmac_context(smctx);
    return 0;
}