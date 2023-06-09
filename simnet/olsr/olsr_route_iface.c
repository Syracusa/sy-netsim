#include "olsr_route_iface.h"

#include <stdio.h>
#include "timerqueue.h"

typedef struct OlsrContext
{
    in_addr_t ip;

    TqCtx *timerqueue;
} OlsrContext;

OlsrContext g_olsr_ctx;

void olsr_route_set_config(RouteConfig *config)
{
    printf("olsr_route_set_config() called\n");
}

void olsr_route_proto_packet_process(void *data, size_t len)
{
    printf("olsr_route_proto_packet_process() called\n");
}

void olsr_route_update_datapkt(void *pkt, size_t *len)
{
    printf("olsr_route_update_datapkt() called\n");
}

void olsr_send_hello()
{
    printf("olsr_send_hello() called\n");
}

void olsr_start()
{
    printf("olsr_start() called\n");
    OlsrContext *ctx = &g_olsr_ctx;
    ctx->timerqueue = create_timerqueue();

    static TqElem send_hello;
    send_hello.arg = NULL;
    send_hello.callback = olsr_send_hello;
    send_hello.use_once = 0;
    send_hello.interval_us = 1000 * 1000;

    timerqueue_register_job(ctx->timerqueue, &send_hello);
}

void olsr_work()
{
    OlsrContext *ctx = &g_olsr_ctx;
    timerqueue_work(ctx->timerqueue);
    // printf("olsr_work() called\n");
}