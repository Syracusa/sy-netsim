#ifndef NET_STATS_H
#define NET_STATS_H

#include "timerqueue.h"
#include "netinet/in.h"

#define MAX_NODE_COUNT 50
#define MAX_HOP_COUNT   50

typedef struct TrafficCountInfo
{
    uint32_t tx_bytes;
    uint32_t rx_bytes;
    uint32_t tx_pkts;
    uint32_t rx_pkts;
    int dirty;
} TrafficCountInfo;

typedef struct RoutingInfo
{
    int hop_count;
    in_addr_t path[MAX_HOP_COUNT];
    int dirty;
} RoutingInfo;

typedef struct NeighborInfo
{
    in_addr_t addr;
    TrafficCountInfo traffic;
    RoutingInfo routing;
} NeighborInfo;

typedef struct NetStats
{
    int node_stats_num;
    NeighborInfo node_info[MAX_NODE_COUNT];
} NetStats;

#endif