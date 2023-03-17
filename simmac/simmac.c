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

    mq_flush(smctx->mqid_send_net);
    mq_flush(smctx->mqid_recv_net);
    mq_flush(smctx->mqid_send_phy);
    mq_flush(smctx->mqid_recv_phy);
}

void sendto_phy(SimMacCtx *smctx, void* data, size_t len, long type)
{
    MqMsgbuf msg;
    msg.type = type;

    if (len > MQ_MAX_DATA_LEN){
        fprintf(stderr, "Can't send data with length %lu\n", len);
    }
    memcpy(msg.text, data, len);
    int ret = msgsnd(smctx->mqid_send_phy, &msg, len, IPC_NOWAIT);
    if (ret < 0){
        if (errno == EAGAIN){
            fprintf(stderr, "Message queue full!\n");
        } else {
            fprintf(stderr, "Can't send to phy(%s)\n", strerror(errno));
        }
    }
}

void recv_mq(SimMacCtx *smctx)
{

}

void mainloop(SimMacCtx *smctx)
{
    struct timespec before, after, diff, req, rem;

    while (1) {
        clock_gettime(CLOCK_REALTIME, &before);

        // send_mq(smctx);
        // recv_mq(smctx);

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
    printf("SIMMAC NODE ID : %u\n", smctx->node_id);
}

int main(int argc, char *argv[])
{
    SimMacCtx *smctx = create_simmac_context();
    parse_arg(smctx, argc, argv);

    mainloop(smctx);

    delete_simmac_context(smctx);
    return 0;
}