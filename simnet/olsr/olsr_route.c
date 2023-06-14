#include "olsr.h"
#include "olsr_route.h"

static void handle_olsr_msg(OlsrMsgHeader* msg, in_addr_t src, size_t msgsize)
{
    /* Check TTL and Forwarding */
    OlsrContext* ctx = &g_olsr_ctx;

    olsr_reltime vtime = me_to_reltime(msg->olsr_vtime);

    /* Process OLSR Routing Messages */
    switch (msg->olsr_msgtype) {
        case MSG_TYPE_HELLO:
            process_olsr_hello(ctx, src, msg->msg_payload, vtime, msgsize);
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
        OlsrMsgHeader* msghdr = (OlsrMsgHeader*)offset;
        size_t msgsize = ntohs(msghdr->olsr_msgsize);
        if (msgsize == 0){
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
