#ifndef OLSR_ROUTE_H
#define OLSR_ROUTE_H

#include <stddef.h>
#include "../../util/netutil.h"

void handle_route_pkt(PktBuf* pkt);
void handle_data_pkt(PktBuf* pkt);

#endif