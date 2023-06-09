#ifndef ROUTE_IFACE_H
#define ROUTE_IFACE_H

#include <stddef.h>
#include <stdint.h>

#include "../util/netutil.h"

typedef void (*route_proto_packet_send)(void *data, size_t len);
typedef struct RouteConfig
{
    route_proto_packet_send sendfn;
    in_addr_t own_ip;
} RouteConfig;

typedef void (*route_set_config)(RouteConfig *config);
typedef void (*route_proto_packet_process)(void *data, size_t len);

typedef void (*route_update_datapkt)(void *pkt, size_t *len);

typedef void (*route_start)();
typedef void (*route_work)();

typedef struct RouteContext
{
    /* Context variable */
    uint16_t route_port;

    /* Functions */
    route_set_config set_config;
    route_proto_packet_process process_route;
    route_update_datapkt update_pkt;
    route_start start;
    route_work work;
} RouteContext;


#endif