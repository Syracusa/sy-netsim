#include "olsr.h"
#include "olsr_route.h"

void olsr_queue_hello(void *olsr_ctx)
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

void olsr_queue_tc(OlsrContext *ctx)
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
