#ifndef OLSR_CONTEXT_H
#define OLSR_CONTEXT_H

#include <netinet/in.h>

#include "timerqueue.h"
#include "ringbuffer.h"
#include "olsr_route_iface.h"
#include "rbtree.h"

#define OLSR_TX_MSGBUF_SIZE 2000

typedef struct LinkElem
{
    rbnode_type priv_rbn;
    in_addr_t neighbor_iface_addr; /* Rbtree Key */

    uint32_t sym_time;
    uint32_t asym_time;
    uint32_t expire_time;
} LinkElem;

typedef struct NeighborLinkElem
{
    rbnode_type priv_rbn;
    in_addr_t local_iface_addr; /* Rbtree Key */

    rbtree_type *neighbor_link_tree; /* Tree of LinkElem */
} NeighborLinkElem;

typedef struct Neighbor2Elem
{
    rbnode_type priv_rbn;
    in_addr_t neighbor2_main_addr; /* Rbtree Key */
    uint32_t expire_time;
} Neighbor2Elem;

typedef struct NeighborElem
{
    rbnode_type priv_rbn;

    in_addr_t neighbor_main_addr; /* Rbtree Key */
    uint8_t status;
    uint8_t willingness;

    rbtree_type *neighbor2_tree; /* Tree of Neighbor2Elem */
} NeighborElem;

typedef struct MprElem
{
    rbnode_type priv_rbn;
    in_addr_t mpr_addr; /* Rbtree Key */
} MprElem;

typedef struct MprSelecorElem
{
    rbnode_type priv_rbn; 
    in_addr_t selector_addr; /* Rbtree Key */
    uint32_t expire_time;
} MprSelecorElem;

typedef struct DestTopologyInfoElem
{
    rbnode_type priv_rbn; 
    in_addr_t last_addr; /* Rbtree Key */
    uint16_t seq;
    uint32_t expire_time;
} DestTopologyInfoElem;

typedef struct TopologyInfoElem
{
    rbnode_type priv_rbn; 
    in_addr_t dest_addr; /* Rbtree Key */
    DestTopologyInfoElem last_tree;
} TopologyInfoElem;

typedef struct OlsrParam
{
    int hello_interval_ms;
    int tc_interval_ms;
    int willingness;
} OlsrParam;

typedef struct OlsrContext
{
    CommonRouteConfig conf;
    OlsrParam param;

    TqCtx *timerqueue;
    RingBuffer *olsr_tx_msgbuf;

    uint16_t pkt_seq;

    rbtree_type *neighbor_link_tree; /* Tree of NeighborLinkElem */
    rbtree_type *neighbor_tree; /* Tree of NeighborElem */
    rbtree_type *mpr_tree; /* Tree of MprElem */
    rbtree_type *selector_tree; /* Tree of MprSelecorElem */
    rbtree_type *topology_tree; /* Tree of TopologyInfoElem */
} OlsrContext;

extern OlsrContext g_olsr_ctx;

void init_olsr_context(CommonRouteConfig *config);
void finalize_olsr_context();
#endif