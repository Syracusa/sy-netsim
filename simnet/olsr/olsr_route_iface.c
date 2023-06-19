#include <stdio.h>

#include "olsr_route_iface.h"
#include "timerqueue.h"
#include "olsr.h"


void olsr_handle_local_pkt(void *data, size_t len)
{
    printf("olsr_handle_local_pkt() called\n");
}

void olsr_handle_remote_pkt(void *data, size_t len)
{
    PktBuf buf;
    ippkt_unpack(&buf, data, len);

    if (buf.iph.protocol == IPPROTO_UDP) {
        uint16_t port = ntohs(buf.udph.dest);

        if (port == OLSR_PROTO_PORT) {
            if (DUMP_ROUTE_PKT) {
                TLOGD("Recv Route Pkt\n");
                hexdump(data, len, stdout);
            }
            handle_route_pkt(&buf);
        } else {
            // fprintf(stderr, "Data pkt with port %u\n", port);
            handle_data_pkt(&buf);
        }
    }
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

    if (DUMP_ROUTE_PKT) {
        TLOGD("Send Route Pkt\n");
        hexdump(buf, sendlen, stdout);
    }
}

static void debug_olsr_context_cb(void *arg)
{
    (void)arg;
    debug_olsr_context();
}

void olsr_start(CommonRouteConfig *config)
{
    printf("olsr_start() called\n");

    init_olsr_context(config);
    OlsrContext *ctx = &g_olsr_ctx;

    static TimerqueueElem *queue_hello = NULL;
    if (queue_hello == NULL)
        queue_hello = timerqueue_new_timer();

    queue_hello->arg = ctx;
    queue_hello->callback = olsr_queue_hello;
    queue_hello->use_once = 0;
    queue_hello->interval_us = ctx->param.hello_interval_ms * 1000;
    queue_hello->max_jitter = ctx->param.hello_interval_ms * 1000 / 8;
    timerqueue_register_timer(ctx->timerqueue, queue_hello);

    static TimerqueueElem *job_tx_msg = NULL;
    if (job_tx_msg == NULL)
        job_tx_msg = timerqueue_new_timer();

    job_tx_msg->arg = ctx;
    job_tx_msg->callback = olsr_send_from_queue;
    job_tx_msg->use_once = 0;
    job_tx_msg->interval_us = 50 * 1000;
    timerqueue_register_timer(ctx->timerqueue, job_tx_msg);

    static TimerqueueElem *debug_timer = NULL;
    if (debug_timer == NULL)
        debug_timer = timerqueue_new_timer();

    debug_timer->arg = NULL;
    debug_timer->callback = debug_olsr_context_cb;
    debug_timer->use_once = 0;
    debug_timer->interval_us = 2000 * 1000;
    debug_timer->max_jitter = 100 * 1000;
    timerqueue_register_timer(ctx->timerqueue, debug_timer);
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