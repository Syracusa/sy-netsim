#include "olsr.h"
#include "olsr_route.h"

void olsr_send_from_queue_cb(void *arg)
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

void olsr_start_timer(OlsrContext* ctx)
{
    static TimerqueueElem *queue_hello = NULL;
    if (queue_hello == NULL)
        queue_hello = timerqueue_new_timer();

    queue_hello->arg = ctx;
    queue_hello->callback = olsr_queue_hello_cb;
    queue_hello->use_once = 0;
    queue_hello->interval_us = ctx->param.hello_interval_ms * 1000;
    queue_hello->max_jitter = ctx->param.hello_interval_ms * 1000 / 8;
    timerqueue_register_timer(ctx->timerqueue, queue_hello);

    static TimerqueueElem *job_tx_msg = NULL;
    if (job_tx_msg == NULL)
        job_tx_msg = timerqueue_new_timer();

    job_tx_msg->arg = ctx;
    job_tx_msg->callback = olsr_send_from_queue_cb;
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

void olsr_queue_hello_cb(void *olsr_ctx)
{
    OlsrContext *ctx = olsr_ctx;

    uint8_t buf[MAX_IPPKT_SIZE];
    uint8_t *offset = buf;

    OlsrMsgHeader *msghdr = (OlsrMsgHeader *)buf;
    msghdr->olsr_msgtype = MSG_TYPE_HELLO;
    msghdr->olsr_vtime = reltime_to_me(DEF_TC_VTIME);
    msghdr->originator = ctx->conf.own_ip;
    msghdr->ttl = 1;
    msghdr->hopcnt = 0;
    msghdr->seqno = htons(ctx->pkt_seq);

    offset += sizeof(OlsrMsgHeader);

    size_t hello_len = MAX_IPPKT_SIZE;
    build_olsr_hello(ctx, offset, &hello_len);
    offset += hello_len;

    uint16_t msglen = offset - buf;

    msghdr->olsr_msgsize = htons(msglen);
    RingBuffer_push(ctx->olsr_tx_msgbuf, buf, msglen);
}

void olsr_queue_tc_cb(OlsrContext *ctx)
{
    uint8_t buf[MAX_IPPKT_SIZE];
    uint8_t *offset = buf;

    OlsrMsgHeader *msghdr = (OlsrMsgHeader *)buf;
    msghdr->olsr_msgtype = MSG_TYPE_TC;
    msghdr->olsr_vtime = reltime_to_me(DEF_TC_VTIME);
    msghdr->originator = ctx->conf.own_ip;
    msghdr->ttl = 64;
    msghdr->hopcnt = 0;
    msghdr->seqno = htons(ctx->pkt_seq);

    offset += sizeof(OlsrMsgHeader);

    size_t tc_len = MAX_IPPKT_SIZE;
    build_olsr_tc(ctx, offset, &tc_len);
    offset += tc_len;

    uint16_t msglen = offset - buf;

    msghdr->olsr_msgsize = htons(msglen);
    RingBuffer_push(ctx->olsr_tx_msgbuf, buf, msglen);
}

static void handle_olsr_msg(OlsrMsgHeader *msg, in_addr_t src, size_t msgsize)
{
    /* Check TTL and Forwarding */
    OlsrContext *ctx = &g_olsr_ctx;

    olsr_reltime vtime = me_to_reltime(msg->olsr_vtime);

    /* Process OLSR Routing Messages */
    switch (msg->olsr_msgtype) {
        case MSG_TYPE_HELLO:
            process_olsr_hello(ctx, src, msg->originator, msg->msg_payload,
                               vtime, msgsize - sizeof(OlsrMsgHeader));
            break;
        case MSG_TYPE_TC:
            /* TODO */
            break;
        default:
            break;
    }

    /* Check if we should forwarding this message */
    
}

void handle_route_pkt(PktBuf *pkt)
{
    // fprintf(stderr, "Routepktlen : %ld\n", pkt->payload_len);
    if (pkt->payload_len <= 0)
        return;

    uint8_t *offset = pkt->payload;

    /* Skip olsr pkt header */
    offset += 4;

    /* Parse olsr msg */
    while (offset - pkt->payload < pkt->payload_len) {
        OlsrMsgHeader *msghdr = (OlsrMsgHeader *)offset;
        size_t msgsize = ntohs(msghdr->olsr_msgsize);
        if (msgsize == 0) {
            TLOGF("Msgsize is 0!\n");
            exit(2);
        }
        handle_olsr_msg(msghdr, pkt->iph.saddr, msgsize);

        offset += msgsize;
    }
}

void handle_data_pkt(PktBuf *pkt)
{


 
}
