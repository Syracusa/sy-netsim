#ifndef OLSR_CONTEXT_H
#define OLSR_CONTEXT_H

#include <netinet/in.h>

#include "timerqueue.h"
#include "ringbuffer.h"
#include "olsr_route_iface.h"

#define OLSR_TX_MSGBUF_SIZE 2000

typedef struct OlsrContext
{
    RouteConfig *conf;
    
    TqCtx *timerqueue;
    RingBuffer* olsr_tx_msgbuf;

    uint16_t tx_seq;
} OlsrContext;

extern OlsrContext g_olsr_ctx;

void init_olsr_context();
#endif