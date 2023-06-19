#include "olsr_mpr.h"
#include "olsr.h"

#define DEBUG_MPR 0

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
    return n1_set;
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
                if (rbtree_search(n1_set, &n2elem->neighbor2_main_addr))
                    continue;

                n2r_elem = (N2ReachabilityTreeElem *)
                    rbtree_search(n2_set, &n2elem->neighbor2_main_addr);
                if (!n2r_elem) {
                    n2r_elem = (N2ReachabilityTreeElem *)
                        malloc(sizeof(N2ReachabilityTreeElem));
                    n2r_elem->priv_rbn.key = &n2r_elem->n2_addr;
                    n2r_elem->n2_addr = n2elem->neighbor2_main_addr;
                    n2r_elem->n1_tree = rbtree_create(rbtree_compare_by_inetaddr);
                }

                AddrSetElem *addr_elem = malloc(sizeof(AddrSetElem));
                addr_elem->addr = nelem->neighbor_main_addr;
                addr_elem->priv_rbn.key = &addr_elem->addr;
                rbtree_insert(n2r_elem->n1_tree, (rbnode_type *)addr_elem);

                rbtree_insert(n2_set, (rbnode_type *)n2r_elem);
            }
        }
    }
    return n2_set;
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
            in_addr_t n1_addr = *((in_addr_t *)n2r_elem->n1_tree->root->key);
            if (DEBUG_MPR)
                TLOGD("New MPR : %s\n", ip2str(n1_addr));
            MprElem *mpr_elem = malloc(sizeof(MprElem));
            mpr_elem->mpr_addr = n1_addr;
            mpr_elem->priv_rbn.key = &mpr_elem->mpr_addr;
            if (!rbtree_insert(selected_mpr_tree, (rbnode_type *)mpr_elem)) {
                /* Already selected MPR */
                TLOGE("Insert fail : %s\n", ip2str(n1_addr));
                free(mpr_elem);
            }
        }
    }

    MprElem *mpr_elem;
    RBTREE_FOR(mpr_elem, MprElem *, selected_mpr_tree)
    {
        /* Add to main mpr tree */
        rbtree_insert(ctx->mpr_tree, (rbnode_type *)mpr_elem);

        /* Remove from N1 set */
        rbtree_delete(n1_set, &mpr_elem->mpr_addr);

        NeighborElem *mpr_neigh =
            (NeighborElem *)rbtree_search(ctx->neighbor_tree, &mpr_elem->mpr_addr);

        RBTREE_FOR(n2elem, Neighbor2Elem *, mpr_neigh->neighbor2_tree)
        {
            /* Remove from N2 set */
            n2r_elem = (N2ReachabilityTreeElem *)rbtree_search(n2_set, &n2elem->neighbor2_main_addr);
            if (n2r_elem) {
                rbtree_delete(n2_set, &n2r_elem->n2_addr);
                free(n2r_elem);
            }
        }
    }

    free(selected_mpr_tree);
}

static int calc_reachability(NeighborElem *neigh,
                             rbtree_type *n2_set)
{
    Neighbor2Elem *n2elem;
    N2ReachabilityTreeElem *n2r_elem;

    int reachability = 0;
    RBTREE_FOR(n2elem, Neighbor2Elem *, neigh->neighbor2_tree)
    {
        /* If 2-hop neighbor is in n2 set, than add reachability */
        if (rbtree_search(n2_set, &n2elem->neighbor2_main_addr))
            reachability++;
    }

    return reachability;
}

static NeighborElem *get_max_reachablilty_neigh(rbtree_type *n1_set,
                                                rbtree_type *n2_set)
{
    int max_reachability = 0;
    NeighborElem *max_reachability_neigh = NULL;
    NeighborElem *nelem;

    RBTREE_FOR(nelem, NeighborElem *, n1_set)
    {
        int reachability = calc_reachability(nelem, n2_set);
        if (reachability > max_reachability) {
            max_reachability = reachability;
            max_reachability_neigh = nelem;
        }
    }

    return max_reachability_neigh;
}

static void populate_mpr_set_by_reachability(OlsrContext *ctx,
                                             rbtree_type *n1_set,
                                             rbtree_type *n2_set)
{
    NeighborElem *nelem;
    Neighbor2Elem *n2elem;

    /* Populate MPR set until no elem left in n2_set */
    while (n2_set->count > 0) {
        nelem = get_max_reachablilty_neigh(n1_set, n2_set);
        if (!nelem) {
            break;
        }

        MprElem *mpr_elem = malloc(sizeof(MprElem));
        mpr_elem->mpr_addr = nelem->neighbor_main_addr;
        mpr_elem->priv_rbn.key = &mpr_elem->mpr_addr;
        rbtree_insert(ctx->mpr_tree, (rbnode_type *)mpr_elem);

        /* Remove from N1 set */
        rbtree_delete(n1_set, &mpr_elem->mpr_addr);

        NeighborElem *mpr_neigh =
            (NeighborElem *)rbtree_search(ctx->neighbor_tree,
                                          &mpr_elem->mpr_addr);

        RBTREE_FOR(n2elem, Neighbor2Elem *, mpr_neigh->neighbor2_tree)
        {
            /* Remove from N2 set */
            N2ReachabilityTreeElem *n2r_elem;
            n2r_elem = (N2ReachabilityTreeElem *)
                rbtree_search(n2_set, &n2elem->neighbor2_main_addr);
            if (n2r_elem) {
                rbtree_delete(n2_set, &n2r_elem->n2_addr);
                free(n2r_elem);
            }
        }
    }
}

void print_n1_set(rbtree_type *n1_set)
{
    if (n1_set->count == 0) {
        TLOGD("N1: empty\n");
        return;
    }
    AddrSetElem *addr;
    RBTREE_FOR(addr, AddrSetElem *, n1_set)
    {
        TLOGD("N1: %s\n", ip2str(addr->addr));
    }
}

void print_n2_set(rbtree_type *n2_set)
{
    if (n2_set->count == 0) {
        TLOGD("N2: empty\n");
        return;
    }

    N2ReachabilityTreeElem *n2r_elem;
    RBTREE_FOR(n2r_elem, N2ReachabilityTreeElem *, n2_set)
    {
        TLOGD("N2: %s\n", ip2str(n2r_elem->n2_addr));
    }
}

void populate_mpr_set(OlsrContext *ctx)
{
    clean_mpr_set(ctx->mpr_tree);
    ctx->mpr_tree = rbtree_create(rbtree_compare_by_inetaddr);

    /* make N set and N2 set */
    rbtree_type *n1_set = calc_n1_set(ctx);
    rbtree_type *n2_set = calc_n2_set(ctx, n1_set);
    if (DEBUG_MPR) {
        print_n1_set(n1_set);
        print_n2_set(n2_set);
    }

    add_unique_bridge_to_mpr(ctx, n1_set, n2_set);
    populate_mpr_set_by_reachability(ctx, n1_set, n2_set);
}
