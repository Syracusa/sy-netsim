#include "olsr_route_iface.h"

#include <stdio.h>
#include "timerqueue.h"
#include "olsr.h"

void olsr_handle_local_pkt(void *data, size_t len)
{
    printf("olsr_route_handle_pkt() called\n");
}

void olsr_handle_remote_pkt(void *data, size_t len)
{
    printf("olsr_route_handle_pkt() called\n");
}

void olsr_queue_hello()
{
    printf("olsr_queue_hello() called\n");
    OlsrContext *ctx = &g_olsr_ctx;

    unsigned char buf[MAX_IPPKT_SIZE];
    size_t hello_len = MAX_IPPKT_SIZE;
    build_olsr_hello(ctx, buf, &hello_len);

    RingBuffer_push(ctx->olsr_tx_msgbuf, buf, hello_len);
}

void olsr_send_from_queue()
{
    OlsrContext *ctx = &g_olsr_ctx;

    PktBuf pkt;
    unsigned char buf[MAX_IPPKT_SIZE];
    ssize_t read = RingBuffer_get_remain_bufsize(ctx->olsr_tx_msgbuf);

    if (read <= 0)
        return;

    /* Payload */
    uint8_t *offset = pkt.payload;

    uint16_t pktlen_be = htons(4 + read);
    memcpy(offset, &pktlen_be, sizeof(pktlen_be));
    offset += sizeof(pktlen_be);

    uint16_t seq_be = htons(ctx->pkt_seq);
    memcpy(offset, &seq_be, sizeof(seq_be));
    offset += sizeof(seq_be);

    RingBuffer_pop(ctx->olsr_tx_msgbuf, offset, read);
    offset += read;

    pkt.payload_len = offset - pkt.payload;

    /* UDP Header */
    build_udp_hdr_no_checksum(&(pkt.udph), OLSR_PROTO_PORT,
                              OLSR_PROTO_PORT, pkt.payload_len);

    /* IP Header */
    MAKE_BE32_IP(broadcast_ip, 255, 255, 255, 255);

    build_ip_hdr(&(pkt.iph), pkt.payload_len + IPUDP_HDRLEN, 64,
                 ctx->conf.own_ip, broadcast_ip, IPPROTO_UDP);
    pkt.iph_len = sizeof(struct iphdr);

    size_t sendlen = MAX_IPPKT_SIZE;
    ippkt_pack(&pkt, buf, &sendlen);

    ctx->conf.send_remote(buf, sendlen);
    ctx->pkt_seq++;
}

void olsr_start(CommonRouteConfig *config)
{
    printf("olsr_start() called\n");

    init_olsr_context(config);
    OlsrContext *ctx = &g_olsr_ctx;

    static TqElem queue_hello;
    queue_hello.arg = NULL;
    queue_hello.callback = olsr_queue_hello;
    queue_hello.use_once = 0;
    queue_hello.interval_us = ctx->param.hello_interval_ms * 1000;
    timerqueue_register_job(ctx->timerqueue, &queue_hello);
    timerqueue_set_jitter(&queue_hello, ctx->param.hello_interval_ms * 1000 / 8);

    static TqElem job_tx_msg;
    job_tx_msg.arg = NULL;
    job_tx_msg.callback = olsr_send_from_queue;
    job_tx_msg.use_once = 0;
    job_tx_msg.interval_us = 50 * 1000;
    timerqueue_register_job(ctx->timerqueue, &job_tx_msg);
}

void olsr_work()
{
    OlsrContext *ctx = &g_olsr_ctx;
    timerqueue_work(ctx->timerqueue);
    // printf("olsr_work() called\n");
}

void olsr_end()
{
    printf("olsr_end() called\n");
    finalize_olsr_context();
}