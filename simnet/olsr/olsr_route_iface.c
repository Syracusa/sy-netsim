#include "olsr_route_iface.h"

#include <stdio.h>
#include "timerqueue.h"
#include "olsr.h"

void olsr_route_proto_packet_process(void *data, size_t len)
{
    printf("olsr_route_proto_packet_process() called\n");
}

void olsr_route_update_datapkt(void *pkt, size_t *len)
{
    printf("olsr_route_update_datapkt() called\n");
}

void olsr_queue_hello()
{
    printf("olsr_queue_hello() called\n");
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

    uint16_t seq_be = htons(ctx->tx_seq);
    memcpy(offset, seq_be, sizeof(seq_be));
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
                 ctx->conf->own_ip, broadcast_ip, IPPROTO_UDP);
    pkt.iph_len = sizeof(struct iphdr);

    size_t sendlen = MAX_IPPKT_SIZE;
    ippkt_pack(&pkt, buf, &sendlen);

    ctx->conf->sendfn(buf, sendlen);
    ctx->tx_seq++;
}

void olsr_start(RouteConfig *config)
{
    printf("olsr_start() called\n");

    init_olsr_context();
    OlsrContext *ctx = &g_olsr_ctx;
    memcpy(ctx->conf, config, sizeof(RouteConfig));
    ctx->timerqueue = create_timerqueue();

    static TqElem queue_hello;
    queue_hello.arg = NULL;
    queue_hello.callback = olsr_queue_hello;
    queue_hello.use_once = 0;
    queue_hello.interval_us = 1000 * 1000;

    static TqElem job_tx_msg;
    job_tx_msg.arg = NULL;
    job_tx_msg.callback = olsr_send_from_queue;
    job_tx_msg.use_once = 0;
    job_tx_msg.interval_us = 50;

    timerqueue_register_job(ctx->timerqueue, &queue_hello);
}

void olsr_work()
{
    OlsrContext *ctx = &g_olsr_ctx;
    timerqueue_work(ctx->timerqueue);
    // printf("olsr_work() called\n");
}