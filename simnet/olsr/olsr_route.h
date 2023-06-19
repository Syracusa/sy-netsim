#ifndef OLSR_ROUTE_H
#define OLSR_ROUTE_H

#include <stddef.h>
#include "../../util/netutil.h"

void handle_route_pkt(PktBuf *pkt);
void handle_data_pkt(PktBuf *pkt);
void olsr_queue_hello_cb(void *olsr_ctx);
void olsr_queue_tc_cb(OlsrContext *ctx);
void olsr_start_timer(OlsrContext* ctx);

#endif