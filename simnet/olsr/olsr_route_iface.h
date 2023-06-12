#ifndef OLSR_ROUTE_IFACE_H
#define OLSR_ROUTE_IFACE_H

#include "olsr_proto.h"
#include "../route_iface.h"

void olsr_route_proto_packet_process(void *data, size_t len);

void olsr_route_update_datapkt(void *pkt, size_t *len);

void olsr_start(RouteConfig *config);

void olsr_work();

static RouteContext olsr_iface = {
    .route_port = OLSR_PROTO_PORT,
    .process_route = olsr_route_proto_packet_process,
    .update_pkt = olsr_route_update_datapkt,
    .start = olsr_start,
    .work = olsr_work
};

#endif