#include "olsr_hello.h"

#include "olsr.h"

#include <stddef.h>

static uint8_t get_neighbor_status(OlsrContext *ctx, in_addr_t addr)
{
    NeighborElem *elem;
    elem = (NeighborElem *)rbtree_search(ctx->neighbor_tree, &addr);

    if (elem != (NeighborElem *)RBTREE_NULL)
        return elem->status;

    return STATUS_UNAVAILABLE;
}

static void write_links_with_code(OlsrContext *ctx, uint8_t **offsetp, uint8_t code)
{
    NeighborLinkElem *nelem;
    uint16_t write_count = 0;

    HelloInfo *hello_info = (HelloInfo *)*offsetp;
    *offsetp += sizeof(HelloInfo);

    RBTREE_FOR(nelem, NeighborLinkElem *, ctx->neighbor_tree)
    {
        LinkElem *lelem;
        RBTREE_FOR(lelem, LinkElem *, nelem->neighbor_link_tree)
        {
            uint8_t neighbor_status =
                get_neighbor_status(ctx, lelem->neighbor_iface_addr);

            if (neighbor_status == STATUS_UNAVAILABLE)
                continue;

            uint8_t link_code = CREATE_LINK_CODE(lelem->status, neighbor_status);

            if (link_code == code) {
                hello_info->neigh_addr[write_count] = lelem->neighbor_iface_addr;
                write_count++;
                *offsetp += sizeof(lelem->neighbor_iface_addr);
            }
        }
    }

    if (write_count == 0) {
        *offsetp -= sizeof(HelloInfo);
    } else {
        hello_info->size = htons(write_count);
    }
}

static void write_links(OlsrContext *ctx, uint8_t **offset)
{
    for (uint8_t nstat = 0; nstat < NEIGHBOR_STATUS_END; nstat++) {
        for (uint8_t lstat = 0; lstat < LINK_STATUS_END; lstat++) {
            uint8_t code = CREATE_LINK_CODE(lstat, nstat);
            write_links_with_code(ctx, offset, code);
        }
    }
}

void build_olsr_hello(OlsrContext *ctx, void *buf, size_t *len)
{
    uint8_t *start = buf;
    uint8_t *offset = buf;

    OlsrMsgHeader *msghdr = (OlsrMsgHeader *)offset;
    msghdr->olsr_msgtype = MSG_TYPE_HELLO;
    msghdr->olsr_vtime = 0; /* TODO */
    msghdr->originator = ctx->conf.own_ip;
    msghdr->ttl = 1;
    msghdr->hopcnt = 1;
    msghdr->seqno = htons(ctx->pkt_seq);

    offset += sizeof(OlsrMsgHeader);

    HelloMsg *hello_msg = (HelloMsg *)offset;
    hello_msg->reserved = 0;
    hello_msg->htime = 0; /* TODO */
    hello_msg->willingness = ctx->param.willingness;

    offset += sizeof(OlsrMsgHeader);

    write_links(ctx, &offset);

    msghdr->olsr_msgsize = htons(offset - start);
}


void process_olsr_hello()
{

}