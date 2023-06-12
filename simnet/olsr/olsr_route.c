#include "olsr.h"
#include "olsr_route.h"

static void handle_olsr_msg(OlsrMsgHeader* msg, in_addr_t src)
{
    /* Check TTL and Forwarding */
    OlsrContext* ctx = &g_olsr_ctx;

    /* Process OLSR Routing Messages */
    switch (msg->olsr_msgtype) {
        case MSG_TYPE_HELLO:
            process_olsr_hello(ctx, src, msg->msg_payload);
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
    if (pkt->payload_len <= 0)
        return;

    uint8_t *offset = pkt->payload;

    while (offset - pkt->payload >= pkt->payload_len) {
        OlsrMsgHeader* msghdr = (OlsrMsgHeader*)offset;
        handle_olsr_msg(msghdr, pkt->iph.saddr);
        offset += msghdr->olsr_msgsize;
    }
}

void handle_data_pkt(PktBuf *pkt)
{

}
