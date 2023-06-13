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
    PktBuf buf;
    ippkt_unpack(&buf, data, len);

    if (buf.udph.dest == ntohs(OLSR_PROTO_PORT)) {
        handle_route_pkt(&buf);
    } else {
        handle_data_pkt(&buf);
    }
}

void olsr_queue_hello(void *olsr_ctx)
{
    OlsrContext *ctx = olsr_ctx;

    uint8_t buf[MAX_IPPKT_SIZE];
    uint8_t *offset = buf;

    OlsrMsgHeader *msghdr = (OlsrMsgHeader *)buf;
    msghdr->olsr_msgtype = MSG_TYPE_HELLO;
    msghdr->olsr_vtime = 0; /* TODO */
    msghdr->originator = ctx->conf.own_ip;
    msghdr->ttl = 1;
    msghdr->hopcnt = 1;
    msghdr->seqno = htons(ctx->pkt_seq);

    offset += sizeof(OlsrMsgHeader);

    size_t hello_len = MAX_IPPKT_SIZE;
    build_olsr_hello(ctx, offset, &hello_len);
    offset += hello_len;

    uint16_t msglen = offset - buf;

    msghdr->olsr_msgsize = htons(msglen);
    RingBuffer_push(ctx->olsr_tx_msgbuf, buf, msglen);
}

void olsr_send_from_queue(void *arg)
{
    OlsrContext *ctx = arg;

    PktBuf pkt;

    ssize_t read = RingBuffer_get_readable_bufsize(ctx->olsr_tx_msgbuf);

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

    unsigned char buf[MAX_IPPKT_SIZE];
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

    static TqElem *queue_hello = NULL;
    if (queue_hello == NULL)
        queue_hello = timerqueue_new_timer();

    queue_hello->arg = ctx;
    queue_hello->callback = olsr_queue_hello;
    queue_hello->use_once = 0;
    queue_hello->interval_us = ctx->param.hello_interval_ms * 1000;
    queue_hello->max_jitter = ctx->param.hello_interval_ms * 1000 / 8;
    timerqueue_register_timer(ctx->timerqueue, queue_hello);

    static TqElem *job_tx_msg = NULL;
    if (job_tx_msg == NULL)
        job_tx_msg = timerqueue_new_timer();

    job_tx_msg->arg = ctx;
    job_tx_msg->callback = olsr_send_from_queue;
    job_tx_msg->use_once = 0;
    job_tx_msg->interval_us = 50 * 1000;
    timerqueue_register_timer(ctx->timerqueue, job_tx_msg);
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