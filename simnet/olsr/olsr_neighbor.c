#include "olsr_neighbor.h"
#include "olsr.h"

uint8_t get_neighbor_status(OlsrContext *ctx,
                            in_addr_t addr)
{
    NeighborElem *elem;
    elem = (NeighborElem *)rbtree_search(ctx->neighbor_tree, &addr);

    if (elem != NULL)
        return elem->status;

    return STATUS_UNAVAILABLE;
}

void set_neighbor_status(NeighborElem *neighbor, uint8_t status)
{
    uint8_t old = neighbor->status;
    if (old != status) {
        TLOGW("Neighbor %s  Status %s -> %s\n",
              ip2str(neighbor->neighbor_main_addr),
              neighbor_status_str(old),
              neighbor_status_str(status));
        neighbor->status = status;
    }
}

NeighborElem *make_neighbor_elem(OlsrContext *ctx,
                                 in_addr_t neigh_addr,
                                 uint8_t willingness)
{
    NeighborElem *neigh = malloc(sizeof(NeighborElem));

    neigh->rbn.key = &neigh->neighbor_main_addr;
    neigh->neighbor_main_addr = neigh_addr;
    neigh->status = STATUS_UNAVAILABLE;
    neigh->willingness = willingness;
    neigh->neighbor2_tree = rbtree_create(rbtree_compare_by_inetaddr);

    return neigh;
}

void neigh2_elem_expire(void *arg)
{
    OlsrContext *ctx = &g_olsr_ctx;
    Neighbor2Elem *n2elem = arg;
    NeighborElem *neigh =
        (NeighborElem *)rbtree_search(ctx->neighbor_tree, &n2elem->bridge_addr);

    TLOGW("2-hop Neighbor %s bridged by %s Expired\n"
          , ip2str(n2elem->neighbor2_main_addr)
          , ip2str(neigh->neighbor_main_addr));

    timerqueue_free_timer(n2elem->expire_timer);
    rbtree_delete(neigh->neighbor2_tree, &n2elem->neighbor2_main_addr);
    free(n2elem);
}

void update_neigh2_tuple(OlsrContext *ctx,
                         NeighborElem *neigh,
                         in_addr_t n2addr,
                         olsr_reltime vtime)
{
    Neighbor2Elem *n2elem =
        (Neighbor2Elem *)rbtree_search(neigh->neighbor2_tree, &n2addr);

    if (n2elem == NULL) {
        Neighbor2Elem *n2elem = (Neighbor2Elem *)malloc(sizeof(Neighbor2Elem));
        n2elem->rbn.key = &n2elem->neighbor2_main_addr;
        n2elem->neighbor2_main_addr = n2addr;
        n2elem->bridge_addr = neigh->neighbor_main_addr;

        n2elem->expire_timer = timerqueue_new_timer();
        n2elem->expire_timer->interval_us = vtime * 1000;
        n2elem->expire_timer->use_once = 1;
        n2elem->expire_timer->arg = n2elem;
        n2elem->expire_timer->callback = neigh2_elem_expire;
        timerqueue_register_timer(ctx->timerqueue, n2elem->expire_timer);
        sprintf(n2elem->expire_timer->debug_name, "neigh2_elem_expire");
        rbtree_insert(neigh->neighbor2_tree, (rbnode_type *)n2elem);
    } else {
        n2elem->expire_timer->interval_us = vtime * 1000;
        timerqueue_reactivate_timer(ctx->timerqueue, n2elem->expire_timer);
    }
}

void update_neighbor_status(OlsrContext *ctx,
                            NeighborElem *neigh)
{
    int neighbor_is_sym = 0;
    LocalNetIfaceElem *ielem;
    RBTREE_FOR(ielem, LocalNetIfaceElem *, ctx->local_iface_tree)
    {
        LinkElem *lelem;
        RBTREE_FOR(lelem, LinkElem *, ielem->iface_link_tree)
        {
            if (lelem->neighbor_iface_addr == neigh->neighbor_main_addr) {
                if (lelem->status == SYM_LINK) {
                    neighbor_is_sym = 1;
                }
            }
        }
    }

    if (neighbor_is_sym) {
        if (neigh->status != MPR_NEIGH)
            set_neighbor_status(neigh, SYM_NEIGH);
    } else {
        set_neighbor_status(neigh, NOT_NEIGH);
    }
}