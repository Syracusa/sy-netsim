#ifndef NET_STATS_H
#define NET_STATS_H

#include "timerqueue.h"
#include "netinet/in.h"

#define MAX_NODE_COUNT 50

typedef struct TrafficCountElem
{
    uint32_t tx_bytes;
    uint32_t rx_bytes;
    uint32_t tx_pkts;
    uint32_t rx_pkts;
} TrafficCountElem;

typedef struct RoutingInfoElem
{
    in_addr_t path[10];
} RoutingInfoElem;

typedef struct NeighborInfo
{
    in_addr_t addr;
    TrafficCountElem traffic;
    RoutingInfoElem routing;
} NeighborInfo;

typedef struct NetStats
{
    int node_stats_num;
    NeighborInfo node_info[MAX_NODE_COUNT];
} NetStats;

#endif