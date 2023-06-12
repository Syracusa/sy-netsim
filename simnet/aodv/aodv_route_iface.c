#include "aodv_route_iface.h"

#include <stdio.h>

void aodv_route_proto_packet_process(void *data, size_t len)
{
    printf("aodv_route_proto_packet_process() called\n");
}

void aodv_route_update_datapkt(void *pkt, size_t *len)
{
    printf("aodv_route_update_datapkt() called\n");
}

void aodv_start(CommonRouteConfig *config){
    printf("aodv_start() called\n");
}

void aodv_work(){
    printf("aodv_work() called\n");
}