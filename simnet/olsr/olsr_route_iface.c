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

static void statistics_update(void *ippkt)
{
    struct iphdr *iph = ippkt;

    OlsrContext *ctx = &g_olsr_ctx;
    NeighborStatInfo *stat_elem = get_neighborstat_buf(ctx, iph->daddr);
    NeighborInfo *info = stat_elem->info;
    info->traffic.tx_bytes += ntohs(iph->tot_len);
    info->traffic.tx_pkts++;
    info->traffic.dirty = 1;
}

void olsr_handle_local_pkt(void *data, size_t len)
{
    OlsrContext *ctx = &g_olsr_ctx;
    if (!ctx)
        return;

    struct iphdr *iph = (struct iphdr *)data;

    if (OLSR_ROUTE_IFACE_VERBOSE)
        TLOGD("Handle packet from local(%s => %s)\n",
              ip2str(iph->saddr), ip2str(iph->daddr));

    PacketBuf pkb;
    pkb.length = len;
    memcpy(pkb.data, data, len);

    handle_local_data_pkt(&pkb);
}

void olsr_handle_remote_pkt(void *data, size_t len)
{
    PacketBuf pkb;
    pkb.length = len;
    memcpy(pkb.data, data, len);

    unsigned char *ptr = pkb.data;

    struct iphdr *iph = (struct iphdr *)pkb.data;
    struct udphdr *udph = (struct udphdr *)(ptr + iph->ihl * 4);
    statistics_update(pkb.data);

    if (OLSR_ROUTE_IFACE_VERBOSE)
        TLOGD("Handle packet from remote(%s <= %s)\n",
              ip2str(iph->daddr), ip2str(iph->saddr));
    
    if (iph->protocol == IPPROTO_UDP) {
        uint16_t port = ntohs(udph->dest);

        if (port == OLSR_PROTO_PORT) {
            if (DUMP_ROUTE_PKT) {
                TLOGD("Recv Route Pkt\n");
                hexdump(pkb.data, pkb.length, stdout);
            }
            handle_route_pkt(&pkb);
            return;
        }
    }

    handle_remote_data_pkt(&pkb);
}

void olsr_start(CommonRouteConfig *config)
{
    if (OLSR_ROUTE_IFACE_VERBOSE)
        TLOGD("olsr_start() called\n");

    init_olsr_context(config);
    OlsrContext *ctx = &g_olsr_ctx;
    olsr_start_timer(ctx);
}

void olsr_work()
{
    OlsrContext *ctx = &g_olsr_ctx;
    timerqueue_work(ctx->timerqueue);
}

void olsr_end()
{
    if (OLSR_ROUTE_IFACE_VERBOSE)
        TLOGD("olsr_end() called\n");

    finalize_olsr_context();
}