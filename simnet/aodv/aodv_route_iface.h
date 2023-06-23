#ifndef AODV_ROUTE_IFACE_H
#define AODV_ROUTE_IFACE_H

#include "aodv_proto.h"

#include "../route_iface.h"

void aodv_handle_local_pkt(void *data, size_t len);

void aodv_handle_remote_pkt(void *data, size_t len);

void aodv_start(CommonRouteConfig *config);

void aodv_work();

void aodv_end();

extern RouteFunctions aodv_iface;

#endif