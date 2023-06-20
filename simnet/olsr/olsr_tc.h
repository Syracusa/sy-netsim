#ifndef OLSR_TC_H
#define OLSR_TC_H

#include "olsr_context.h"

void build_olsr_tc(OlsrContext *ctx,
                   void *buf,
                   size_t *len);

void process_olsr_tc(OlsrContext *ctx,
                     TcMsg *msg,
                     uint16_t tc_size,
                     in_addr_t src,
                     in_addr_t orig,
                     olsr_reltime vtime);

#endif