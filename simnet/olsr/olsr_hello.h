#ifndef OLSR_HELLO_H
#define OLSR_HELLO_H

#include <stddef.h>
#include "olsr_context.h"

void build_olsr_hello(OlsrContext *ctx, void *buf, size_t *len);

#endif