#ifndef OLSR_HELLO_H
#define OLSR_HELLO_H

#include <stddef.h>
#include "olsr_context.h"
#include "olsr_mantissa.h"

void build_olsr_hello(OlsrContext *ctx,
                      void *buf,
                      size_t *len);

void process_olsr_hello(OlsrContext *ctx,
                        in_addr_t src,
                        void *hello,
                        olsr_reltime vtime,
                        size_t msglen);

#endif