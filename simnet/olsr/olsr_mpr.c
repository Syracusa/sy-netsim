#include "olsr_mpr.h"
#include "olsr.h"

typedef struct AddrSetElem {
    rbnode_type priv_rbn;
    in_addr_t addr;
} AddrSetElem;

typedef struct N2ReachabilityTreeElem {
    rbnode_type priv_rbn;
    in_addr_t n2_addr;
    rbtree_type *n1_tree;
} N2ReachabilityTreeElem;

static void free_arg(rbnode_type *rbn, void *dummy)
{
    (void)dummy;
    free(rbn);
}

void clean_mpr_set(rbtree_type *tree)
{
    traverse_postorder(tree, free_arg, NULL);
    free(tree);

}

static rbtree_type *calc_n1_set(OlsrContext *ctx)
{
    NeighborElem *nelem;
    rbtree_type *n1_set = rbtree_create(rbtree_compare_by_inetaddr);
    RBTREE_FOR(nelem, NeighborElem *, ctx->neighbor_tree)
    {
        if (nelem->status == SYM_NEIGH ||
            nelem->status == MPR_NEIGH) {

            AddrSetElem *addr_elem = malloc(sizeof(AddrSetElem));
            addr_elem->addr = nelem->neighbor_main_addr;
            addr_elem->priv_rbn.key = &addr_elem->addr;
            rbtree_insert(n1_set, (rbnode_type *)addr_elem);

            nelem->status == SYM_NEIGH; /* MPR will be recalculated */
        }
    }
}

static rbtree_type *calc_n2_set(OlsrContext *ctx,
                                rbtree_type *n1_set)
{
    NeighborElem *nelem;
    Neighbor2Elem *n2elem;
    N2ReachabilityTreeElem *n2r_elem;

    rbtree_type *n2_set = rbtree_create(rbtree_compare_by_inetaddr);
    RBTREE_FOR(nelem, NeighborElem *, ctx->neighbor_tree)
    {
        if (nelem->status == SYM_NEIGH) {

            RBTREE_FOR(n2elem, Neighbor2Elem *, nelem->neighbor2_tree)
            {
                if (n2elem->neighbor2_main_addr == ctx->conf.own_ip)
                    continue;

                /* Not truly 2-hop(also 1-hop) */
                if (rbtree_search(n1_set, n2elem->neighbor2_main_addr))
                    continue;

                n2r_elem = rbtree_search(n2_set, &n2elem->neighbor2_main_addr);
                if (!n2r_elem) {
                    n2r_elem = (N2ReachabilityTreeElem *)
                        malloc(sizeof(N2ReachabilityTreeElem));

                    n2r_elem->n2_addr = n2elem->neighbor2_main_addr;
                    n2r_elem->n1_tree = rbtree_create(rbtree_compare_by_inetaddr);
                }

                AddrSetElem *addr_elem = malloc(sizeof(AddrSetElem));
                addr_elem->addr = nelem->neighbor_main_addr;
                addr_elem->priv_rbn.key = &addr_elem->addr;
                rbtree_insert(n2r_elem->n1_tree, addr_elem);

                rbtree_insert(n2_set, (rbnode_type *)n2r_elem);
            }
        }
    }
}

/*
Add to the MPR set those nodes in N, which are the *only*
nodes to provide reachability to a node in N2
*/
static void add_unique_bridge_to_mpr(OlsrContext *ctx,
                                     rbtree_type *n1_set,
                                     rbtree_type *n2_set)
{
    NeighborElem *nelem;
    Neighbor2Elem *n2elem;
    N2ReachabilityTreeElem *n2r_elem;

    rbtree_type *selected_mpr_tree = rbtree_create(rbtree_compare_by_inetaddr);
    RBTREE_FOR(n2r_elem, N2ReachabilityTreeElem *, n2_set)
    {
        if (n2r_elem->n1_tree->count == 1) {
            MprElem *mpr_elem = malloc(sizeof(mpr_elem));
            mpr_elem->mpr_addr = *((in_addr_t *)n2r_elem->n1_tree->root->key);
            mpr_elem->priv_rbn.key = &mpr_elem->mpr_addr;
            if (!rbtree_insert(selected_mpr_tree, mpr_elem)) {
                /* Already selected MPR */
                free(mpr_elem);
            }
        }
    }

    MprElem *mpr_elem;
    RBTREE_FOR(mpr_elem, MprElem *, selected_mpr_tree)
    {
        /* Remove from N1 set */
        rbtree_delete(n1_set, &mpr_elem->mpr_addr);

        NeighborElem *mpr_neigh = rbtree_search(ctx->neighbor_tree,
                                                &mpr_elem->mpr_addr);

        RBTREE_FOR(n2elem, Neighbor2Elem *, mpr_neigh->neighbor2_tree)
        {
            /* Remove from N2 set */
            n2r_elem = rbtree_search(n2_set, &n2elem->neighbor2_main_addr);
            if (n2r_elem) {
                rbtree_delete(n2_set, &n2r_elem->n2_addr);
                free(n2r_elem);
            }
        }
    }

    clean_mpr_set(selected_mpr_tree);
}

void populate_mpr_set(OlsrContext *ctx)
{
    clean_mpr_set(ctx->mpr_tree);
    ctx->mpr_tree = rbtree_create(rbtree_compare_by_inetaddr);
    NeighborElem *nelem;

    /* make N set and N2 set */
    rbtree_type *n1_set = calc_n1_set(ctx);
    rbtree_type *n2_set = calc_n2_set(ctx, n1_set);
    add_unique_bridge_to_mpr(ctx, n1_set, n2_set);

    /* TODO */
}