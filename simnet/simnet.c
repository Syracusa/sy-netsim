#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <errno.h>

#include "mq.h"
#include "util.h"
#include "timerqueue.h"
#include "netutil.h"

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
    int ret = msgsnd(snctx->mqid_send_mac, &msg, len, IPC_NOWAIT);
    if (ret < 0) {
        if (errno == EAGAIN) {
            TLOGE("Message queue full!\n");
        } else {
            TLOGE("Can't send to mac. mqid: %d len: %lu(%s)\n",
                  snctx->mqid_send_mac, len, strerror(errno));
        }
    }
}

void process_mac_msg(SimNetCtx *snctx, void *data, int len)
{
    PktBuf pkb;
    memcpy(&pkb.iph, data, len);
    pkb.payload_len = len - IPUDP_HDRLEN;

    if (PKT_HEXDUMP)
        hexdump(data, len, stdout);
    
    TLOGD("Recv pkt. %s <- %s(Payloadsz: %ld)\n",
          ip2str(pkb.iph.daddr), ip2str(pkb.iph.saddr), pkb.payload_len);
}

void recvfrom_mac(SimNetCtx *snctx)
{
    MqMsgbuf msg;
    while (1) {
        ssize_t res = msgrcv(snctx->mqid_recv_mac, &msg, sizeof(msg.text), 0, IPC_NOWAIT);
        if (res < 0) {
            if (errno != ENOMSG) {
                TLOGE("Msgrcv failed(err: %s)\n", strerror(errno));
            }
            break;
        }
        process_mac_msg(snctx, msg.text, res);
    }
}

void mainloop(SimNetCtx *snctx)
{
    struct timespec before, after, diff, req, rem;

    while (1) {
        clock_gettime(CLOCK_REALTIME, &before);

        timerqueue_work(snctx->timerqueue);
        recvfrom_mac(snctx);

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
    printf("SIMNET NODE ID : %u\n", snctx->node_id);
}

static void send_dummy_packet(void *arg)
{
    SimNetCtx *snctx = arg;

    PktBuf pkb;
    memset(&pkb, 0x00, sizeof(PktBuf));

    pkb.payload_len = 100;

    MAKE_BE32_IP(sender_ip, 192, 168, snctx->node_id, 1);
    MAKE_BE32_IP(receiver_ip, 192, 168, snctx->node_id + 1, 1);

    build_ip_hdr(&(pkb.iph), pkb.payload_len + IPUDP_HDRLEN, 64,
                 sender_ip, receiver_ip, IPPROTO_UDP);
    build_udp_hdr_no_checksum(&(pkb.udph), 29111, 29112, pkb.payload_len);

    TLOGD("Send dummy pkt. %s -> %s(Payloadsz: %ld)\n",
          ip2str(pkb.iph.saddr), ip2str(pkb.iph.daddr), pkb.payload_len);

    if (PKT_HEXDUMP)
        hexdump(&pkb.iph, pkb.payload_len + IPUDP_HDRLEN, stdout);
    
    sendto_mac(snctx, &(pkb.iph), 100 + IPUDP_HDRLEN, 1);
}

static void send_dummy_log_to_simulator(void *arg)
{
    // fprintf(stderr, "TODO : Send dummy log to simulator\n");
}

static void register_works(SimNetCtx *snctx)
{
    static TqElem dummy_pkt_gen;
    dummy_pkt_gen.arg = snctx;
    dummy_pkt_gen.callback = send_dummy_packet;
    dummy_pkt_gen.use_once = 0;
    dummy_pkt_gen.interval_us = 1000000;

    timerqueue_register_job(snctx->timerqueue, &dummy_pkt_gen);

    static TqElem dummy_log_gen;
    dummy_log_gen.arg = snctx;
    dummy_log_gen.callback = send_dummy_log_to_simulator;
    dummy_log_gen.use_once = 0;
    dummy_log_gen.interval_us = 1000000;

    timerqueue_register_job(snctx->timerqueue, &dummy_log_gen);
}

int main(int argc, char *argv[])
{
    SimNetCtx *snctx = create_simnet_context();
    parse_arg(snctx, argc, argv);
    sprintf(dbgname, "NET-%-2d", snctx->node_id);

    init_mq(snctx);

    register_works(snctx);
    mainloop(snctx);

    delete_simnet_context(snctx);
    return 0;
}