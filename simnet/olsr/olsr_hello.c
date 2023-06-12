#include "olsr_hello.h"

#include "olsr.h"

#include <stddef.h>

void build_olsr_hello(OlsrContext *ctx, void *buf, size_t *len)
{
    OlsrMsgHeader *msghdr = (OlsrMsgHeader *)buf;
    msghdr->olsr_msgtype = MSG_TYPE_HELLO;
    msghdr->olsr_vtime = 0; /* TODO */
    msghdr->originator = ctx->conf.own_ip;
    msghdr->ttl = 1;
    msghdr->hopcnt = 1;
    msghdr->seqno = ctx->pkt_seq;


    msghdr->olsr_msgsize = 0;
}