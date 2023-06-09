#ifndef OLSR_ROUTE_IFACE_H
#define OLSR_ROUTE_IFACE_H

#include "olsr_proto.h"

#include "../route_iface.h"

void olsr_route_set_proto_sendfn(route_proto_packet_send sendfn);

void olsr_route_proto_packet_process(void *data, size_t len);

void olsr_route_update_datapkt(void *pkt, size_t *len);

void olsr_start();

static RouteContext olsr_iface = {
    .route_port = OLSR_PROTO_PORT,
    .set_sendfn = olsr_route_set_proto_sendfn,
    .process_route = olsr_route_proto_packet_process,
    .update_pkt = olsr_route_update_datapkt,
    .start = olsr_start
};

#endif