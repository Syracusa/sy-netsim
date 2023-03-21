#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <errno.h>

#include "mq.h"
#include "util.h"
#include "timerqueue.h"

#include "log.h"
#include "params.h"



typedef struct
{
    TqCtx *timerqueue;
    int node_id;

    int mqid_recv_mac;
    int mqid_send_mac;
} SimNetCtx;

void init_mq(SimNetCtx *snctx)
{
    int mqkey_send_mac = MQ_KEY_NET_TO_MAC + snctx->node_id;
    snctx->mqid_send_mac = msgget(mqkey_send_mac, IPC_CREAT | 0666);

    int mqkey_recv_mac = MQ_KEY_MAC_TO_NET + snctx->node_id;
    snctx->mqid_recv_mac = msgget(mqkey_recv_mac, IPC_CREAT | 0666);

    mq_flush(snctx->mqid_send_mac);
    mq_flush(snctx->mqid_recv_mac);
}

void sendto_mac(SimNetCtx *snctx, void *data, size_t len, long type)
{
    MqMsgbuf msg;
    msg.type = type;

    if (len > MQ_MAX_DATA_LEN) {
        fprintf(stderr, "Can't send data with length %lu\n", len);
    }
    memcpy(msg.text, data, len);
    int ret = msgsnd(snctx->mqid_send_mac, &msg, len, IPC_NOWAIT);
    if (ret < 0) {
        if (errno == EAGAIN) {
            fprintf(stderr, "Message queue full!\n");
        } else {
            fprintf(stderr, "Can't send to phy(%s)\n", strerror(errno));
        }
    }
}

void recvfrom_mac(SimNetCtx *snctx)
{

}

void mainloop(SimNetCtx *snctx)
{
    struct timespec before, after, diff, req, rem;

    while (1) {
        clock_gettime(CLOCK_REALTIME, &before);

        timerqueue_work(snctx->timerqueue);

        clock_gettime(CLOCK_REALTIME, &after);
        timespec_sub(&after, &before, &diff);
        if (diff.tv_nsec < 1000 * 1000) {
            req.tv_sec = 0;
            req.tv_nsec = 1000 * 1000 - diff.tv_nsec;
            nanosleep(&req, &rem);
        }
    }
}

static SimNetCtx *create_simnet_context()
{
    SimNetCtx *snctx = malloc(sizeof(SimNetCtx));
    memset(snctx, 0x00, sizeof(SimNetCtx));

    snctx->timerqueue = create_timerqueue();
    return snctx;
}

static void delete_simnet_context(SimNetCtx *snctx)
{
    free(snctx);
}

static void parse_arg(SimNetCtx *snctx, int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage : ./%s {NODE_ID}\n", argv[0]);
        exit(2);
    }

    int res = sscanf(argv[1], "%u", &(snctx->node_id));

    if (res < 1) {
        fprintf(stderr, "Can't parse nodeID from [%s]\n", argv[1]);
        exit(2);
    }
    printf("SIMMAC NODE ID : %u\n", snctx->node_id);
}

static void send_dummy_packet(void *arg)
{
    SimNetCtx *snctx = arg;
    fprintf(stderr, "Dummy send test! nid : %u\n", snctx->node_id);
}

static void register_works(SimNetCtx *snctx)
{
    static TqElem dummygen;
    dummygen.arg = snctx;
    dummygen.callback = send_dummy_packet;
    dummygen.use_once = 0;
    dummygen.interval_us = 1000000;

    timerqueue_register_job(snctx->timerqueue, &dummygen);
}

int main(int argc, char *argv[])
{
    SimNetCtx *snctx = create_simnet_context();
    parse_arg(snctx, argc, argv);

    register_works(snctx);

    mainloop(snctx);

    delete_simnet_context(snctx);
    return 0;
}