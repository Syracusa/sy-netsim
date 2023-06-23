#include "aodv_route_iface.h"

#include <stdio.h>

RouteFunctions aodv_iface = {
    .handle_local_pkt = aodv_handle_local_pkt,
    .handle_remote_pkt = aodv_handle_remote_pkt,
    .start = aodv_start,
    .work = aodv_work,
    .end = aodv_end
};

void aodv_handle_local_pkt(void *data, size_t len)
{
    printf("aodv_handle_local_pkt() called\n");
}

void aodv_handle_remote_pkt(void *pkt, size_t len)
{
    printf("aodv_handle_remote_pkt() called\n");
}

void aodv_start(CommonRouteConfig *config){
    printf("aodv_start() called\n");
}

void aodv_work(){
    printf("aodv_work() called\n");
}

void aodv_end(){
    printf("aodv_end() called\n");
}