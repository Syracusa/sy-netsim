#include "olsr.h"

static void duplicate_neigh_info(OlsrContext *ctx)
{
    /* Duplicate neighbor info to each node */
    TopologyInfoElem *telem;
    RBTREE_FOR(telem, TopologyInfoElem *, ctx->topology_tree)
    {
        AdvertisedNeighElem *aelem;
        RBTREE_FOR(aelem, AdvertisedNeighElem *, telem->an_tree)
        {
            TopologyInfoElem *peer = (TopologyInfoElem *)
                rbtree_search(ctx->topology_tree, &aelem->last_addr);
            if (!peer) {
                TLOGE("TopologyInfoElem not found for %s\n",
                      ip2str(aelem->last_addr));
                continue;
            }
            rbtree_insert(peer->an_tree, (rbnode_type *)&telem->dest_addr);
        }
    }
}

typedef struct VisitQueueElem {
    CllElem l;
    RoutingEntry *route_entry;
} VisitQueueElem;

static void routing_table_add_one_hop_neigh(OlsrContext *ctx, CllHead *visit_queue)
{
    VisitQueueElem *vqelem;
    NeighborElem *nelem;
    RBTREE_FOR(nelem, NeighborElem *, ctx->neighbor_tree)
    {
        if (nelem->status == SYM_NEIGH || nelem->status == MPR_NEIGH) {
            RoutingEntry *entry = (RoutingEntry *)malloc(sizeof(RoutingEntry));
            entry->rbn.key = &entry->dest_addr;
            entry->dest_addr = nelem->neighbor_main_addr;
            entry->hop_count = 1;

            AddrListElem *elem = (AddrListElem *)malloc(sizeof(AddrListElem));
            elem->addr = nelem->neighbor_main_addr;
            cll_init_head(&entry->route);
            cll_add_tail(&entry->route, (CllElem *)elem);
            rbtree_insert(ctx->routing_table, (rbnode_type *)entry);

            vqelem = (VisitQueueElem *)malloc(sizeof(VisitQueueElem));
            vqelem->route_entry = entry;

            cll_add_tail(visit_queue, (CllElem *)vqelem);
        }
    }
}

void calc_routing_table(OlsrContext *ctx)
{
    duplicate_neigh_info(ctx);

    CllHead visit_queue;
    cll_init_head(&visit_queue);


    /* Add all one-hop symmetric neighbor to routing table */
    routing_table_add_one_hop_neigh(ctx, &visit_queue);

    while (!cll_no_entry(&visit_queue)) {
        VisitQueueElem *vqelem = (VisitQueueElem *)cll_pop_head(&visit_queue);
        RoutingEntry *curr_entry = vqelem->route_entry;
        TopologyInfoElem *telem = (TopologyInfoElem *)
            rbtree_search(ctx->topology_tree, &curr_entry->dest_addr);

        if (!telem) {
            TLOGE("TopologyInfoElem not found for %s\n",
                  ip2str(curr_entry->dest_addr));
            continue;
        }

        AdvertisedNeighElem *aelem;
        RBTREE_FOR(aelem, AdvertisedNeighElem *, telem->an_tree)
        {
            if (aelem->last_addr == ctx->conf.own_ip) {
                continue;
            }

            RoutingEntry *candidate_entry = (RoutingEntry *)
                rbtree_search(ctx->routing_table, &aelem->last_addr);

            if (!candidate_entry) {
                RoutingEntry *new_entry = (RoutingEntry *)
                    malloc(sizeof(RoutingEntry));
                new_entry->rbn.key = &new_entry->dest_addr;
                new_entry->dest_addr = aelem->last_addr;
                new_entry->hop_count = curr_entry->hop_count + 1;

                /* Copy list */
                CllElem* elem;
                cll_foreach(elem, &curr_entry->route) {
                    AddrListElem *new_elem = (AddrListElem *)
                        malloc(sizeof(AddrListElem));
                    new_elem->addr = ((AddrListElem *)elem)->addr;
                    cll_add_tail(&new_entry->route, (CllElem *)new_elem);
                }

                /* Add dest */
                AddrListElem *new_elem = (AddrListElem *)
                    malloc(sizeof(AddrListElem));
                new_elem->addr = aelem->last_addr;
                cll_add_tail(&new_entry->route, (CllElem *)new_elem);


                // AddrListElem *elem;
                // cll_init_head(&new_entry->route);
                // elem->addr = curr_entry->dest_addr;

                // cll_add_tail(&new_entry->route, (CllElem *)elem);

                /* Add dest */

            }
        }

    }


}