#ifndef AODV_ROUTE_IFACE_H
#define AODV_ROUTE_IFACE_H

#include "aodv_proto.h"

#include "../route_iface.h"

void aodv_route_set_proto_sendfn(route_proto_packet_send sendfn);

void aodv_route_proto_packet_process(void *data, size_t len);

void aodv_route_update_datapkt(void *pkt, size_t *len);

void aodv_start();

static RouteContext aodv_iface = {
    .route_port = AODV_PROTO_PORT,
    .set_sendfn = aodv_route_set_proto_sendfn,
    .process_route = aodv_route_proto_packet_process,
    .update_pkt = aodv_route_update_datapkt,
    .start = aodv_start
};

#endif