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

    uint16_t seq_be = htons(ctx->msg_seq);
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
    ctx->msg_seq++;

    if (DUMP_ROUTE_PKT) {
        TLOGD("Send Route Pkt\n");
        hexdump(buf, sendlen, stdout);
    }
}

static void dump_debuginfo_cb(void *arg)
{
    (void)arg;
    dump_olsr_context();
    dump_statistics();
}

void olsr_start_timer(OlsrContext *ctx)
{
    static TimerqueueElem *hello_timer = NULL;
    if (hello_timer == NULL)
        hello_timer = timerqueue_new_timer();

    hello_timer->arg = ctx;
    hello_timer->callback = olsr_hello_timer_cb;
    hello_timer->use_once = 0;
    hello_timer->interval_us = ctx->param.hello_interval_ms * 1000;
    hello_timer->max_jitter = ctx->param.hello_interval_ms * 1000 / 8;
    sprintf(hello_timer->debug_name, "olsr_hello_timer_cb");
    timerqueue_register_timer(ctx->timerqueue, hello_timer);

    static TimerqueueElem *tc_timer = NULL;
    if (tc_timer == NULL)
        tc_timer = timerqueue_new_timer();

    tc_timer->arg = ctx;
    tc_timer->callback = olsr_tc_timer_cb;
    tc_timer->use_once = 0;
    tc_timer->interval_us = ctx->param.tc_interval_ms * 1000;
    tc_timer->max_jitter = ctx->param.tc_interval_ms * 1000 / 8;
    sprintf(tc_timer->debug_name, "olsr_tc_timer_cb");
    timerqueue_register_timer(ctx->timerqueue, tc_timer);

    static TimerqueueElem *job_tx_msg = NULL;
    if (job_tx_msg == NULL)
        job_tx_msg = timerqueue_new_timer();

    job_tx_msg->arg = ctx;
    job_tx_msg->callback = olsr_send_from_queue_cb;
    job_tx_msg->use_once = 0;
    job_tx_msg->interval_us = 50 * 1000;
    sprintf(job_tx_msg->debug_name, "olsr_send_from_queue_cb");
    timerqueue_register_timer(ctx->timerqueue, job_tx_msg);

    static TimerqueueElem *debug_timer = NULL;
    if (debug_timer == NULL)
        debug_timer = timerqueue_new_timer();

    debug_timer->arg = NULL;
    debug_timer->callback = dump_debuginfo_cb;
    debug_timer->use_once = 0;
    debug_timer->interval_us = 6000 * 1000;
    debug_timer->max_jitter = 100 * 1000;
    sprintf(debug_timer->debug_name, "dump_debuginfo_cb");
    timerqueue_register_timer(ctx->timerqueue, debug_timer);
}

void olsr_hello_timer_cb(void *olsr_ctx)
{
    OlsrContext *ctx = olsr_ctx;

    uint8_t buf[MAX_IPPKT_SIZE];
    uint8_t *offset = buf;

    OlsrMsgHeader *msghdr = (OlsrMsgHeader *)buf;
    msghdr->olsr_msgtype = MSG_TYPE_HELLO;
    msghdr->olsr_vtime = reltime_to_me(DEF_HELLO_VTIME);
    msghdr->originator = ctx->conf.own_ip;
    msghdr->ttl = 1;
    msghdr->hopcnt = 0;
    msghdr->seqno = htons(ctx->msg_seq);

    offset += sizeof(OlsrMsgHeader);

    size_t hello_len = MAX_IPPKT_SIZE;
    build_olsr_hello(ctx, offset, &hello_len);
    offset += hello_len;

    uint16_t msglen = offset - buf;

    msghdr->olsr_msgsize = htons(msglen);
    RingBuffer_push(ctx->olsr_tx_msgbuf, buf, msglen);
}

void olsr_tc_timer_cb(void *olsr_ctx)
{
    OlsrContext *ctx = olsr_ctx;

    uint8_t buf[MAX_IPPKT_SIZE];
    uint8_t *offset = buf;

    OlsrMsgHeader *msghdr = (OlsrMsgHeader *)buf;
    msghdr->olsr_msgtype = MSG_TYPE_TC;
    msghdr->olsr_vtime = reltime_to_me(DEF_TC_VTIME);
    msghdr->originator = ctx->conf.own_ip;
    msghdr->ttl = 64;
    msghdr->hopcnt = 0;
    msghdr->seqno = htons(ctx->msg_seq);

    offset += sizeof(OlsrMsgHeader);

    size_t tc_len = MAX_IPPKT_SIZE;
    build_olsr_tc(ctx, offset, &tc_len);
    offset += tc_len;

    uint16_t msglen = offset - buf;

    msghdr->olsr_msgsize = htons(msglen);
    RingBuffer_push(ctx->olsr_tx_msgbuf, buf, msglen);
}

static int is_already_processed(OlsrContext *ctx,
                                OlsrMsgHeader *msg)
{
    DuplicateSetKey dkey;
    dkey.orig = msg->originator;
    dkey.seq = ntohs(msg->seqno);

    DuplicateSetElem *delem = (DuplicateSetElem *)
        rbtree_search(ctx->dup_tree, &dkey);

    if (delem)
        return 1;
    return 0;
}

static void process_olsr_msg(OlsrContext *ctx,
                             OlsrMsgHeader *msg,
                             in_addr_t src)
{
    if (is_already_processed(ctx, msg)) {
        TLOGD("Drop Duplicated Msg From %s(Orignator %s)\n",
              ip2str(src), ip2str(msg->originator));
        return;
    }
    olsr_reltime vtime = me_to_reltime(msg->olsr_vtime);
    uint16_t msgsize = ntohs(msg->olsr_msgsize);
    uint16_t payloadsize = msgsize - sizeof(OlsrMsgHeader);

    /* Process OLSR Routing Messages */
    switch (msg->olsr_msgtype) {
        case MSG_TYPE_HELLO:
            process_olsr_hello(ctx, src, msg->originator, msg->msg_payload,
                               vtime, payloadsize);
            break;
        case MSG_TYPE_TC:
            process_olsr_tc(ctx, (TcMsg *)msg->msg_payload, payloadsize,
                            src, msg->originator, vtime);
            break;
        default:
            break;
    }
}

static void handle_olsr_msg(OlsrMsgHeader *msg,
                            in_addr_t src)
{
    OlsrContext *ctx = &g_olsr_ctx;
    if (msg->originator == ctx->conf.own_ip)
        return;

    process_olsr_msg(ctx, msg, src);

    if (msg->ttl > 1)
        olsr_msg_forwarding(ctx, msg, src);
#if 0
    else
        TLOGD("Drop by TTL (src: %s)\n", ip2str(src));
#endif
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
        handle_olsr_msg(msghdr, pkt->iph.saddr);

        offset += msgsize;
    }
}

void handle_data_pkt(PktBuf *pkt)
{



}
