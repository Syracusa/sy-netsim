#ifndef ROUTE_IFACE_H
#define ROUTE_IFACE_H

#include <stddef.h>
#include <stdint.h>

typedef void (*route_proto_packet_send)(void* data, size_t len);
typedef void (*route_set_proto_sendfn)(route_proto_packet_send sendfn);
typedef void (*route_proto_packet_process)(void* data, size_t len);

typedef void (*route_update_datapkt)(void* pkt, size_t* len);
typedef void (*route_start)();

typedef struct RouteContext{
    /* Context variable */
    uint16_t route_port;
    
    /* Functions */
    route_set_proto_sendfn set_sendfn;
    route_proto_packet_process process_route;
    route_update_datapkt update_pkt;
    route_start start;
} RouteContext;


#endif