#include "aodv_route_iface.h"

#include <stdio.h>

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