#include "aodv_route_iface.h"

#include <stdio.h>

void aodv_route_set_proto_sendfn(route_proto_packet_send sendfn)
{
    printf("aodv_route_set_proto_sendfn() called\n");
}

void aodv_route_proto_packet_process(void *data, size_t len)
{
    printf("aodv_route_proto_packet_process() called\n");
}

void aodv_route_update_datapkt(void *pkt, size_t *len)
{
    printf("aodv_route_update_datapkt() called\n");
}

void aodv_start(){
    printf("aodv_start() called\n");
}