#ifndef OLSR_ROUTE_IFACE_H
#define OLSR_ROUTE_IFACE_H

#include "olsr_proto.h"
#include "../route_iface.h"

void olsr_handle_local_pkt(void *data, size_t len);

void olsr_handle_remote_pkt(void *data, size_t len);

void olsr_start(CommonRouteConfig *config);

void olsr_work();

void olsr_end();

static RouteFunctions olsr_iface = {
    .handle_local_pkt = olsr_handle_local_pkt,
    .handle_remote_pkt = olsr_handle_remote_pkt,
    .start = olsr_start,
    .work = olsr_work,
    .end = olsr_end
};

#endif