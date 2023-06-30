#ifndef ROUTE_IFACE_H
#define ROUTE_IFACE_H

#include <stddef.h>
#include <stdint.h>

#include "./include/net_statistics.h"
#include "../util/netutil.h"

typedef void (*route_send_data)(void *data, size_t len);

typedef struct CommonRouteConfig
{
    route_send_data send_remote;
    route_send_data send_local;
    in_addr_t own_ip;
    NetStats *stat;
} CommonRouteConfig;

typedef void (*route_handle_pkt)(void *data, size_t len);

typedef void (*route_start)(CommonRouteConfig *config);
typedef void (*route_work)();
typedef void (*route_end)();

typedef struct RouteFunctions
{
    route_handle_pkt handle_remote_pkt;
    route_handle_pkt handle_local_pkt;
    route_start start;
    route_work work;
    route_end end;
} RouteFunctions;


#endif