#ifndef AODV_ROUTE_IFACE_H
#define AODV_ROUTE_IFACE_H

#include "aodv_proto.h"

#include "../route_iface.h"

void aodv_handle_local_pkt(void *data, size_t len);

void aodv_handle_remote_pkt(void *data, size_t len);

void aodv_start(CommonRouteConfig *config);

void aodv_work();

void aodv_end();

static RouteContext aodv_iface = {
    .route_port = AODV_PROTO_PORT,
    .handle_local_pkt = aodv_handle_local_pkt,
    .handle_remote_pkt = aodv_handle_remote_pkt,
    .start = aodv_start,
    .work = aodv_work,
    .end = aodv_end
};

#endif