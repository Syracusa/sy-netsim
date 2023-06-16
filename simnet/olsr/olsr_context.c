#include "olsr_context.h"

#include <string.h>
#include "olsr.h"

OlsrContext g_olsr_ctx;

void init_olsr_param()
{
    OlsrParam *param = &g_olsr_ctx.param;

    param->hello_interval_ms = DEF_HELLO_INTERVAL_MS;
    param->tc_interval_ms = DEF_TC_INTERVAL_MS;
    param->willingness = WILL_DEFAULT;
}

int rbtree_compare_by_inetaddr(const void *k1, const void *k2)
{
    in_addr_t *n1 = (in_addr_t *)k1;
    in_addr_t *n2 = (in_addr_t *)k2;

    return *n1 - *n2;
}

void init_olsr_context(CommonRouteConfig *config)
{
    memset(&g_olsr_ctx, 0x00, sizeof(g_olsr_ctx));

    memcpy(&g_olsr_ctx.conf, config, sizeof(CommonRouteConfig));

    init_olsr_param();

    g_olsr_ctx.timerqueue = create_timerqueue();
    g_olsr_ctx.olsr_tx_msgbuf = RingBuffer_new(OLSR_TX_MSGBUF_SIZE);

    g_olsr_ctx.local_iface_tree = rbtree_create(rbtree_compare_by_inetaddr);
    g_olsr_ctx.neighbor_tree = rbtree_create(rbtree_compare_by_inetaddr);
    g_olsr_ctx.mpr_tree = rbtree_create(rbtree_compare_by_inetaddr);
    g_olsr_ctx.selector_tree = rbtree_create(rbtree_compare_by_inetaddr);
    g_olsr_ctx.topology_tree = rbtree_create(rbtree_compare_by_inetaddr);
}

static void free_arg(rbnode_type *rbn, void *dummy)
{
    (void)dummy;
    free(rbn);
}

void finalize_olsr_context()
{
    traverse_postorder(g_olsr_ctx.local_iface_tree, free_arg, NULL);
    free(g_olsr_ctx.local_iface_tree);
    traverse_postorder(g_olsr_ctx.neighbor_tree, free_arg, NULL);
    free(g_olsr_ctx.neighbor_tree);
    traverse_postorder(g_olsr_ctx.mpr_tree, free_arg, NULL);
    free(g_olsr_ctx.mpr_tree);
    traverse_postorder(g_olsr_ctx.selector_tree, free_arg, NULL);
    free(g_olsr_ctx.selector_tree);
    traverse_postorder(g_olsr_ctx.topology_tree, free_arg, NULL);
    free(g_olsr_ctx.topology_tree);
}

void debug_olsr_context()
{
    OlsrContext *ctx = &g_olsr_ctx;

    char logbuf[1000];
    char *offset = logbuf;

    offset += sprintf(offset, "\n==== Context of %-14s ====\n",
                      ip2str(ctx->conf.own_ip));

    LocalNetIfaceElem *ielem;
    RBTREE_FOR(ielem, LocalNetIfaceElem *, ctx->local_iface_tree)
    {
        offset += sprintf(offset, "> Local Iface %s\n",
                          ip2str(ielem->local_iface_addr));
        LinkElem *lelem;
        RBTREE_FOR(lelem, LinkElem *, ielem->iface_link_tree)
        {
            offset += sprintf(offset, ">     Link to %s Status %s\n",
                              ip2str(lelem->neighbor_iface_addr),
                              link_status_str(lelem->status));
        }
    }

    NeighborElem *nelem;
    RBTREE_FOR(nelem, NeighborElem *, ctx->neighbor_tree)
    {
        offset += sprintf(offset, "> Neighbor Node %s Status %s\n",
                          ip2str(nelem->neighbor_main_addr),
                          neighbor_status_str(nelem->status));

        Neighbor2Elem *n2elem;
        RBTREE_FOR(n2elem, Neighbor2Elem *, nelem->neighbor2_tree)
        {
            offset += sprintf(offset, ">     2hop neighbor %s\n",
                              ip2str(n2elem->neighbor2_main_addr));
        }
    }
    offset += sprintf(offset, "\n");

    TLOGD("%s", logbuf);
}