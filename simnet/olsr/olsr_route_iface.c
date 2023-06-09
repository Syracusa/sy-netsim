#include "olsr_route_iface.h"

#include <stdio.h>

void olsr_route_set_proto_sendfn(route_proto_packet_send sendfn)
{
    printf("olsr_route_set_proto_sendfn() called\n");
}

void olsr_route_proto_packet_process(void *data, size_t len)
{
    printf("olsr_route_proto_packet_process() called\n");
}

void olsr_route_update_datapkt(void *pkt, size_t *len)
{
    printf("olsr_route_update_datapkt() called\n");
}

void olsr_start(){
    printf("olsr_start() called\n");
}