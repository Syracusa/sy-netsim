#ifndef NET_STATS_H
#define NET_STATS_H

#include "timerqueue.h"

typedef struct TrafficCountElem
{
    rbnode_type rbn;
    in_addr_t addr; /* Key */
    uint32_t tx_bytes;
    uint32_t rx_bytes;
    uint32_t tx_pkts;
    uint32_t rx_pkts;
} TrafficCountElem;

typedef struct NetStats
{
    rbtree_type *node_stats; /* Elem : TrafficCountElem */
} NetStats;

#endif