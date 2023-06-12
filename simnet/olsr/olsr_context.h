#ifndef OLSR_CONTEXT_H
#define OLSR_CONTEXT_H

#include <netinet/in.h>

#include "timerqueue.h"
#include "ringbuffer.h"
#include "olsr_route_iface.h"

#define OLSR_TX_MSGBUF_SIZE 2000

typedef struct OlsrParam
{
    int hello_interval_ms;
    int tc_interval_ms;
    int willingness;
} OlsrParam;

typedef struct OlsrContext
{
    CommonRouteConfig conf;
    OlsrParam param;
    
    TqCtx *timerqueue;
    RingBuffer* olsr_tx_msgbuf;

    uint16_t pkt_seq;
} OlsrContext;

extern OlsrContext g_olsr_ctx;

void init_olsr_context(CommonRouteConfig *config);
#endif