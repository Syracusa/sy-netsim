#include <stdio.h>

#include "olsr_route_iface.h"
#include "timerqueue.h"
#include "olsr.h"
#include "olsr_context.h"

#define OLSR_ROUTE_IFACE_VERBOSE 0

RouteFunctions olsr_iface = {
    .handle_local_pkt = olsr_handle_local_pkt,
    .handle_remote_pkt = olsr_handle_remote_pkt,
    .start = olsr_start,
    .work = olsr_work,
    .end = olsr_end
};

void olsr_handle_local_pkt(void *data, size_t len)
{
    if (OLSR_ROUTE_IFACE_VERBOSE)
        TLOGD("olsr_handle_local_pkt() called\n");
}

static void statistics_update(PktBuf *buf)
{
    OlsrContext *ctx = &g_olsr_ctx;
    NeighborStatInfo *stat_elem = get_neighborstat_buf(ctx, buf->iph.daddr);
    NeighborInfo *info = stat_elem->info;
    info->traffic.tx_bytes += ntohs(buf->iph.tot_len);
    info->traffic.tx_pkts++;
    info->traffic.dirty = 1;
}

void olsr_handle_remote_pkt(void *data, size_t len)
{
    if (OLSR_ROUTE_IFACE_VERBOSE)
        TLOGD("olsr_handle_remote_pkt() called\n");

    PktBuf buf;
    ippkt_unpack(&buf, data, len);
    statistics_update(&buf);

    if (buf.iph.protocol == IPPROTO_UDP) {
        uint16_t port = ntohs(buf.udph.dest);

        if (port == OLSR_PROTO_PORT) {
            if (DUMP_ROUTE_PKT) {
                TLOGD("Recv Route Pkt\n");
                hexdump(data, len, stdout);
            }
            handle_route_pkt(&buf);
        } else {
            handle_data_pkt(&buf);
        }
    }
}

void olsr_start(CommonRouteConfig *config)
{
    if (OLSR_ROUTE_IFACE_VERBOSE)
        printf("olsr_start() called\n");

    init_olsr_context(config);
    OlsrContext *ctx = &g_olsr_ctx;
    olsr_start_timer(ctx);
}

void olsr_work()
{
    if (OLSR_ROUTE_IFACE_VERBOSE)
        printf("olsr_work() called\n");

    OlsrContext *ctx = &g_olsr_ctx;
    timerqueue_work(ctx->timerqueue);
}

void olsr_end()
{
    if (OLSR_ROUTE_IFACE_VERBOSE)
        printf("olsr_end() called\n");

    finalize_olsr_context();
}