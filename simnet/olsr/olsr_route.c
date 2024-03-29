#include "olsr.h"
#include "olsr_route.h"

void olsr_send_from_queue_cb(void *arg)
{
    OlsrContext *ctx = arg;

    PacketBuf pkt;

    ssize_t read = RingBuffer_get_readable_bufsize(ctx->olsr_tx_msgbuf);

    if (read <= 0)
        return;
    int payload_len = read + 4/* OLSR PACKET HDR */;
    uint8_t *offset = pkt.data;

    /* IP Header */
    MAKE_BE32_IP(broadcast_ip, 255, 255, 255, 255);
    build_ip_hdr(offset, IPUDP_HDRLEN + payload_len, 64,
                 ctx->conf.own_ip, broadcast_ip, IPPROTO_UDP);
    offset += sizeof(struct iphdr);

    /* UDP Header */
    build_udp_hdr_no_checksum(offset, OLSR_PROTO_PORT,
                              OLSR_PROTO_PORT, payload_len);
    offset += sizeof(struct udphdr);

    /* Payload */
    uint16_t pktlen_be = htons(4 + read);
    memcpy(offset, &pktlen_be, sizeof(pktlen_be));
    offset += sizeof(pktlen_be);

    uint16_t seq_be = htons(ctx->msg_seq);
    memcpy(offset, &seq_be, sizeof(seq_be));
    offset += sizeof(seq_be);

    RingBuffer_pop(ctx->olsr_tx_msgbuf, offset, read);
    offset += read;

    pkt.length = offset - pkt.data;

    ctx->conf.send_remote(pkt.data, pkt.length);
    ctx->msg_seq++;

    if (DUMP_ROUTE_PKT) {
        TLOGD("Send Route Pkt\n");
        hexdump(pkt.data, pkt.length, stdout);
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

void handle_route_pkt(PacketBuf *pkt)
{
    const struct iphdr *iph = (struct iphdr *)pkt->data;

    const int payload_len = pkt->length - IPUDP_HDRLEN;

    if (payload_len <= 0) {
        TLOGF("Payload len is 0!\n");
        return;
    }

    const uint8_t *payload_offset = &pkt->data[IPUDP_HDRLEN];
    uint8_t *offset = (uint8_t *)payload_offset;

    /* Skip olsr pkt header */
    offset += 4;

    /* Parse olsr msg */
    while (offset - payload_offset < payload_len) {
        OlsrMsgHeader *msghdr = (OlsrMsgHeader *)offset;
        size_t msgsize = ntohs(msghdr->olsr_msgsize);
        if (msgsize == 0) {
            TLOGF("Msgsize is 0!\n");
            exit(2);
        }
        handle_olsr_msg(msghdr, iph->saddr);

        offset += msgsize;
    }
}

static void ip_route(OlsrContext* ctx, PacketBuf *pkt, int from_local)
{
    struct iphdr *iph = (struct iphdr *)pkt->data;

    /* Get route */
    RoutingEntry *entry =
        (RoutingEntry *)rbtree_search(ctx->routing_table, &iph->daddr);

    if (!entry) {
        TLOGD("No route entry exist for %s\n", ip2str(iph->daddr));
        return;
    }

    TLOGD("Route to %s => %d hop\n", ip2str(iph->daddr), entry->hop_count);

    if (entry->hop_count == 1 && from_local) {
        /* If onehop and local packet, just send to mac */
        ctx->conf.send_remote(pkt->data, pkt->length);
        return;
    }

    /* Multi-hop routing required. Encapsulate packet and send to mac */
    AddrListElem *nexthop_elem = (AddrListElem *)entry->route.next;
    in_addr_t nexthop = nexthop_elem->addr;

    PacketBuf encap_pkt;
    minimal_mip_encap(pkt, &encap_pkt, ctx->conf.own_ip, nexthop);
    ctx->conf.send_remote(encap_pkt.data, encap_pkt.length);
}

void handle_remote_data_pkt(PacketBuf *pkt)
{
    OlsrContext *ctx = &g_olsr_ctx;
    if (!ctx) {
        TLOGF("Context is NULL! - Can't handle local data packet\n");
        return;
    }
    struct iphdr *iph = (struct iphdr *)pkt->data;

    /* Check c-class ip matching */
    if (!IP_C_CLASS_MATCH(ctx->conf.own_ip, iph->daddr)) {
        TLOGD("Drop packet from %s to %s\n",
              ip2str(iph->saddr), ip2str(iph->daddr));
        return;
    }

    switch (iph->protocol) {
        case IPPROTO_MOBILE:
        {
            /* Encapsulated packet. First decapsulate it */
            PacketBuf decap_pkt;
            minimal_mip_decap(pkt, &decap_pkt);

            struct iphdr *decap_iph = (struct iphdr *)decap_pkt.data;
            if (IP_C_CLASS_MATCH(ctx->conf.own_ip, decap_iph->daddr)){
                /* If decapsulated packet is for me, send to local */
                ctx->conf.send_local(decap_pkt.data, decap_pkt.length);
                return;
            }
            
            /* Not for me. Search routing table and forwarding it */
            ip_route(ctx, &decap_pkt, 0);
            
            break;
        }
        default:
            /* Normal packet - send to local */
            ctx->conf.send_local(pkt->data, pkt->length);
            break;
    }
}

void handle_local_data_pkt(PacketBuf *pkt)
{
    OlsrContext *ctx = &g_olsr_ctx;
    if (!ctx) {
        TLOGF("Context is NULL! - Can't handle local data packet\n");
        return;
    }

    struct iphdr *iph = (struct iphdr *)pkt->data;

    /* Do not send local network packet */
    if (IP_C_CLASS_MATCH(ctx->conf.own_ip, iph->daddr))
        return;

    ip_route(ctx, pkt, 1);
}

