#include "olsr_hello.h"
#include "olsr.h"

static uint8_t get_neighbor_status(OlsrContext *ctx,
                                   in_addr_t addr)
{
    NeighborElem *elem;
    elem = (NeighborElem *)rbtree_search(ctx->neighbor_tree, &addr);

    if (elem != (NeighborElem *)RBTREE_NULL)
        return elem->status;

    return STATUS_UNAVAILABLE;
}

static void write_links_with_code(OlsrContext *ctx,
                                  uint8_t **offsetp,
                                  uint8_t code)
{
    LocalNetIfaceElem *ielem;
    uint16_t write_count = 0;

    HelloInfo *hello_info = (HelloInfo *)*offsetp;
    *offsetp += sizeof(HelloInfo);

    RBTREE_FOR(ielem, LocalNetIfaceElem *, ctx->neighbor_tree)
    {
        LinkElem *lelem;
        RBTREE_FOR(lelem, LinkElem *, ielem->local_iface_tree)
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

static void write_links(OlsrContext *ctx,
                        uint8_t **offset)
{
    for (uint8_t nstat = 0; nstat < NEIGHBOR_STATUS_END; nstat++) {
        for (uint8_t lstat = 0; lstat < LINK_STATUS_END; lstat++) {
            uint8_t code = CREATE_LINK_CODE(lstat, nstat);
            write_links_with_code(ctx, offset, code);
        }
    }
}

void build_olsr_hello(OlsrContext *ctx,
                      void *buf,
                      size_t *len)
{
    uint8_t *start = buf;
    uint8_t *offset = buf;

    HelloMsg *hello_msg = (HelloMsg *)offset;
    hello_msg->reserved = 0;
    hello_msg->htime = 0; /* TODO */
    hello_msg->willingness = ctx->param.willingness;

    offset += sizeof(HelloMsg);

    write_links(ctx, &offset);
    *len = offset - start;
}

void process_olsr_hello(OlsrContext *ctx,
                        in_addr_t src, void *hello)
{
    HelloMsg *hello_msg = hello;

    LocalNetIfaceElem *iface =
        (LocalNetIfaceElem *)rbtree_search(ctx->local_iface_tree,
                                           &ctx->conf.own_ip);
                                           
    if (iface == (LocalNetIfaceElem *)RBTREE_NULL) {
        iface = malloc(sizeof(LocalNetIfaceElem));
        memset(iface, 0x00, sizeof(LocalNetIfaceElem));
        iface->priv_rbn.key = &(iface->local_iface_addr);
        iface->local_iface_addr = ctx->conf.own_ip;
        iface->local_iface_tree = rbtree_create(rbtree_compare_by_inetaddr);
        rbtree_insert(ctx->local_iface_tree, (rbnode_type *)iface);
    }

    LinkElem *link = (LinkElem *)rbtree_search(iface->local_iface_tree, &src);
    if (link == (LinkElem *)RBTREE_NULL) {
        link = malloc(sizeof(LinkElem));
        memset(link, 0x00, sizeof(LinkElem));

        /* TODO (7.1.1.  HELLO Message Processing) */
    }

    /*
    TODO
    Upon receiving a HELLO message, the "validity time" MUST be computed
    from the Vtime field of the message header (see section 3.3.2).
    */


}