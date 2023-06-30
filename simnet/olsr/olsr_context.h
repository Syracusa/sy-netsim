#ifndef OLSR_CONTEXT_H
#define OLSR_CONTEXT_H

#include <netinet/in.h>

#include "../include/net_statistics.h"
#include "timerqueue.h"
#include "ringbuffer.h"
#include "olsr_route_iface.h"
#include "rbtree.h"
#include "olsr_mantissa.h"
#include "cll.h"

#define OLSR_TX_MSGBUF_SIZE 2000

typedef struct LinkElem
{
    rbnode_type rbn;
    in_addr_t neighbor_iface_addr; /* Rbtree Key */
    in_addr_t local_iface_addr;

    uint8_t status;

    TimerqueueElem *sym_timer;
    TimerqueueElem *asym_timer;
    TimerqueueElem *expire_timer;
} LinkElem;

typedef struct LocalNetIfaceElem
{
    rbnode_type rbn;
    in_addr_t local_iface_addr; /* Rbtree Key */

    rbtree_type *iface_link_tree; /* Tree of LinkElem */
} LocalNetIfaceElem;

typedef struct Neighbor2Elem
{
    rbnode_type rbn;
    in_addr_t neighbor2_main_addr; /* Rbtree Key */
    in_addr_t bridge_addr;
    TimerqueueElem *expire_timer;
} Neighbor2Elem;

typedef struct NeighborElem
{
    rbnode_type rbn;

    in_addr_t neighbor_main_addr; /* Rbtree Key */
    uint8_t status;
    uint8_t willingness;

    rbtree_type *neighbor2_tree; /* Tree of Neighbor2Elem */
} NeighborElem;

typedef struct MprElem
{
    rbnode_type rbn;
    in_addr_t mpr_addr; /* Rbtree Key */
} MprElem;

typedef struct MprSelectorElem
{
    rbnode_type rbn;
    in_addr_t selector_addr; /* Rbtree Key */
    TimerqueueElem *expire_timer;
} MprSelectorElem;

typedef struct AdvertisedNeighElem
{
    rbnode_type rbn;
    in_addr_t last_addr; /* Rbtree Key */
    int obsolete;
} AdvertisedNeighElem;

typedef struct TopologyInfoElem
{
    rbnode_type rbn;
    in_addr_t dest_addr; /* Rbtree Key */
    uint16_t seq;
    rbtree_type *an_tree; /* Tree of AdvertisedNeighElem */
    TimerqueueElem *expire_timer;
} TopologyInfoElem;

typedef struct DuplicateSetKey
{
    in_addr_t orig;
    uint16_t seq;
} DuplicateSetKey;

typedef struct DuplicateSetElem
{
    rbnode_type rbn;
    DuplicateSetKey key;
    int retransmitted;
    CllHead iface_addr_list;
    TimerqueueElem *expire_timer;
} DuplicateSetElem;

typedef struct OlsrParam
{
    int hello_interval_ms;
    int tc_interval_ms;
    int willingness;
} OlsrParam;

typedef struct AddrListElem
{
    CllElem elem;
    in_addr_t addr;
} AddrListElem;

typedef struct RoutingEntry
{
    rbnode_type rbn;
    in_addr_t dest_addr; /* Rbtree Key */
    int hop_count;
    CllHead route;
} RoutingEntry;

typedef struct NeighborStatInfo
{
    rbnode_type rbn;
    in_addr_t addr; /* Rbtree Key */
    NeighborInfo* info;
} NeighborStatInfo;

typedef struct OlsrContext
{
    CommonRouteConfig conf;
    OlsrParam param;

    TimerqueueContext *timerqueue;
    RingBuffer *olsr_tx_msgbuf;

    uint16_t msg_seq;
    uint16_t ansn; /* Advertised Neighbor Sequence Number */

    rbtree_type *node_stat_tree; /* Tree of NeighborStatInfo */

    rbtree_type *local_iface_tree; /* Tree of LocalNetIfaceElem */
    rbtree_type *neighbor_tree; /* Tree of NeighborElem */
    rbtree_type *mpr_tree; /* Tree of MprElem */
    rbtree_type *selector_tree; /* Tree of MprSelectorElem */
    rbtree_type *topology_tree; /* Tree of TopologyInfoElem */
    rbtree_type *dup_tree; /* Tree of DuplicateSetElem */
    rbtree_type *routing_table; /* Tree of RoutingEntry */
} OlsrContext;

extern OlsrContext g_olsr_ctx;

void init_olsr_context(CommonRouteConfig *config);
void finalize_olsr_context();

int rbtree_compare_by_inetaddr(const void *k1, const void *k2);


void dump_olsr_context();
void dump_statistics();

NeighborStatInfo *get_neighborstat_buf(OlsrContext *ctx, in_addr_t dest);
#endif