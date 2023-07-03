#include "olsr.h"


static void unduplicate_neigh_info(OlsrContext *ctx)
{
    TopologyInfoElem *telem;
    RBTREE_FOR(telem, TopologyInfoElem *, ctx->topology_tree)
    {
        AdvertisedNeighElem *duplicates[50];
        int dup_count = 0;
        AdvertisedNeighElem *aelem;
        RBTREE_FOR(aelem, AdvertisedNeighElem *, telem->an_tree)
        {
            if (aelem->duplicated == 1){
                duplicates[dup_count] = aelem;
                dup_count++;
            }
        }

        for (int i = 0; i < dup_count; i++ ){
            rbtree_delete(telem->an_tree, &duplicates[i]->last_addr);
            free(duplicates[i]);
        }
    }
}


static void duplicate_neigh_info(OlsrContext *ctx)
{
    /* Duplicate neighbor info to each node */
    TopologyInfoElem *telem;
    RBTREE_FOR(telem, TopologyInfoElem *, ctx->topology_tree)
    {
        AdvertisedNeighElem *aelem;
        RBTREE_FOR(aelem, AdvertisedNeighElem *, telem->an_tree)
        {
            if (aelem->last_addr == ctx->conf.own_ip)
                continue;
            TopologyInfoElem *peer = (TopologyInfoElem *)
                rbtree_search(ctx->topology_tree, &aelem->last_addr);
            if (!peer) {
                TLOGE("TopologyInfoElem not found for %s\n",
                      ip2str(aelem->last_addr));
                continue;
            }

            AdvertisedNeighElem *aelem2 = (AdvertisedNeighElem *)
                malloc(sizeof(AdvertisedNeighElem));
            aelem2->last_addr = telem->dest_addr;
            aelem2->rbn.key = &aelem2->last_addr;
            aelem2->obsolete = 0;
            aelem2->duplicated = 1;

            if (!rbtree_insert(peer->an_tree, (rbnode_type *)aelem2))
                free(aelem2);
        }
    }
}

typedef struct VisitQueueElem {
    CllElem l;
    RoutingEntry *route_entry;
} VisitQueueElem;


static void update_route_info_disconncted(OlsrContext *ctx,
                                          rbtree_type *old_table,
                                          rbtree_type *new_table)
{
    RoutingEntry *old_entry;
    RBTREE_FOR(old_entry, RoutingEntry *, old_table)
    {
        /* If entry is not in new_table then disconnected */
        RoutingEntry *new_entry = (RoutingEntry *)
            rbtree_search(new_table, &old_entry->dest_addr);
        if (!new_entry) {
            NeighborStatInfo *nstat = get_neighborstat_buf(ctx, old_entry->dest_addr);
            RoutingInfo *rinfo = &nstat->info->routing;
            rinfo->hop_count = 0; /* Disconnected */
            rinfo->dirty = 1;
        }
    }
}

static void update_route_info_changed(OlsrContext *ctx,
                                      rbtree_type *old_table,
                                      rbtree_type *new_table)
{
    RoutingEntry *new_entry;
    RBTREE_FOR(new_entry, RoutingEntry *, new_table)
    {
        RoutingEntry *old_entry = (RoutingEntry *)
            rbtree_search(old_table, &new_entry->dest_addr);

        if (old_entry) {
            AddrListElem *new_route = (AddrListElem *)new_entry->route.next;
            AddrListElem *old_route = (AddrListElem *)old_entry->route.next;

            int changed = 0;
            for (int i = 0; i < new_entry->hop_count; i++) {
                if (new_route->addr != old_route->addr) {
                    changed = 1;
                    break;
                }
                new_route = (AddrListElem *)new_route->elem.next;
                old_route = (AddrListElem *)old_route->elem.next;
            }

            if (!changed)
                continue;
        }

        NeighborStatInfo *nstat = get_neighborstat_buf(ctx, new_entry->dest_addr);
        RoutingInfo *rinfo = &nstat->info->routing;
        rinfo->hop_count = new_entry->hop_count;
        rinfo->dirty = 1;

        CllHead *elem;
        int entry_idx = 0;
        cll_foreach(elem, &new_entry->route)
        {
            AddrListElem *addr_elem = (AddrListElem *)elem;
            rinfo->path[entry_idx++] = addr_elem->addr;
        }
    }
}

static void update_route_info(OlsrContext *ctx,
                              rbtree_type *old_table,
                              rbtree_type *new_table)
{
    update_route_info_disconncted(ctx, old_table, new_table);
    update_route_info_changed(ctx, old_table, new_table);
}

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

static void drop_routing_table(rbtree_type *routing_table)
{
    RoutingEntry *entry;
    RBTREE_FOR(entry, RoutingEntry *, routing_table)
    {
        AddrListElem *elem;
        while (!cll_no_entry(&entry->route)) {
            elem = (AddrListElem *)cll_pop_head(&entry->route);
            free(elem);
        }
    }
    traverse_postorder(routing_table, free_arg, NULL);
    free(routing_table);
}

/* Calculate routing table form collected information */
void calc_routing_table(OlsrContext *ctx)
{
    TLOGI("Calculate routing table\n");

    rbtree_type *old_table = ctx->routing_table;
    ctx->routing_table = rbtree_create(rbtree_compare_by_inetaddr);
    /* Advertised neighbor info might include only
       one-way as it can advertise MPR selector nodes only..
       So we duplicate's one neighbor info to oppnent node's data
       for convinience */
    duplicate_neigh_info(ctx);


    /* Initialize visit queue to do carry out SPF  */
    CllHead visit_queue;
    cll_init_head(&visit_queue);

    /* Add all one-hop symmetric neighbor to routing table */
    routing_table_add_one_hop_neigh(ctx, &visit_queue);

    while (!cll_no_entry(&visit_queue)) {
        VisitQueueElem *vqelem = (VisitQueueElem *)cll_pop_head(&visit_queue);
        RoutingEntry *curr_entry = vqelem->route_entry;

        if (curr_entry->dest_addr == ctx->conf.own_ip)
            continue;

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

            /* If the neighbor of the entry is not in routing table, Add it */
            RoutingEntry *candidate_entry = (RoutingEntry *)
                rbtree_search(ctx->routing_table, &aelem->last_addr);
            TLOGD("Check connection of %s... %s\n", ip2str(curr_entry->dest_addr),
                  ip2str(aelem->last_addr));

            if (!candidate_entry) {
                RoutingEntry *new_entry = (RoutingEntry *)
                    malloc(sizeof(RoutingEntry));
                new_entry->rbn.key = &new_entry->dest_addr;
                new_entry->dest_addr = aelem->last_addr;
                new_entry->hop_count = curr_entry->hop_count + 1;
                cll_init_head(&new_entry->route);

                TLOGI("New route entry %s -> %s (%d hop)\n",
                      ip2str(curr_entry->dest_addr),
                      ip2str(new_entry->dest_addr),
                      new_entry->hop_count);

                /* Copy list */
                CllElem *elem;
                cll_foreach(elem, &curr_entry->route)
                {
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

                /* Insert to routing table */
                rbtree_insert(ctx->routing_table, (rbnode_type *)new_entry);

                /* Add to visit queue to find further nodes */
                VisitQueueElem *vqelem = (VisitQueueElem *)
                    malloc(sizeof(VisitQueueElem));
                vqelem->route_entry = new_entry;
                cll_add_tail(&visit_queue, (CllElem *)vqelem);
            } else {
                TLOGD("Route to %s already exists\n",
                      ip2str(aelem->last_addr));
            }
        }
    }

    unduplicate_neigh_info(ctx);
    update_route_info(ctx, old_table, ctx->routing_table);
    drop_routing_table(old_table);
}