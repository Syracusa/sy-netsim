#ifndef ROUTE_IFACE_H
#define ROUTE_IFACE_H

#include <stddef.h>
#include <stdint.h>

#include "../util/netutil.h"

typedef void (*route_proto_packet_send)(void *data, size_t len);
typedef struct CommonRouteConfig
{
    route_proto_packet_send sendfn;
    in_addr_t own_ip;
} CommonRouteConfig;

typedef void (*route_proto_packet_process)(void *data, size_t len);

typedef void (*route_update_datapkt)(void *pkt, size_t *len);

typedef void (*route_start)(CommonRouteConfig *config);
typedef void (*route_work)();
typedef void (*route_end)();

typedef struct RouteContext
{
    /* Context variable */
    uint16_t route_port;

    /* Functions */
    route_proto_packet_process process_route;
    route_update_datapkt update_pkt;
    route_start start;
    route_work work;
    route_end end;
} RouteContext;


#endif