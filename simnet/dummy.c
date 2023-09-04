#include "simnet.h"
#include "mac_connection.h"
#include "dummy.h"

static void send_dummy_packet(void *arg)
{
    NetSetDummyTrafficConfig *conf = arg;
    SimNetCtx *snctx = g_snctx;

    PacketBuf pkb;
    MAKE_BE32_IP(sender_ip, 192, 168, snctx->node_id, 1);
    MAKE_BE32_IP(receiver_ip, 192, 168, conf->dst_id, 1);

    unsigned char *ptr = pkb.data;
    build_ip_hdr(ptr, conf->payload_size + IPUDP_HDRLEN, 64,
                 sender_ip, receiver_ip, IPPROTO_UDP);
    ptr += sizeof(struct iphdr);

    build_udp_hdr_no_checksum(ptr, 29111, 29112, conf->payload_size);
    ptr += sizeof(struct udphdr);

    memset(ptr, 0x00, conf->payload_size);
    ptr += conf->payload_size;

    pkb.length = ptr - &pkb.data[0];

    if (DEBUG_NET_TRX) 
        TLOGD("Send dummy pkt. %s -> %s(Payloadsz: %u)\n",
              ip2str(sender_ip), ip2str(receiver_ip), conf->payload_size);
    
    snctx->route->handle_local_pkt(&pkb.data, pkb.length);
}

static void dummypkt_send_job_detach_cb(void *arg)
{
    NetSetDummyTrafficConfig *conf = arg;
    free(conf);
}

void register_dummypkt_send_job(SimNetCtx *snctx,
                                NetSetDummyTrafficConfig *conf)
{
    NetSetDummyTrafficConfig *copy_conf = malloc(sizeof(NetSetDummyTrafficConfig));
    memcpy(copy_conf, conf, sizeof(NetSetDummyTrafficConfig));

    TLOGI("DUMMY STREAM ConfID:%u Dst:%u, Src: %u,"
          " Interval: %u, Payload size: %u\n",
          conf->conf_id, conf->dst_id, conf->src_id,
          conf->interval_ms, conf->payload_size);

    if (!snctx->dummy_traffic_timers[conf->conf_id])
        snctx->dummy_traffic_timers[conf->conf_id] = timerqueue_new_timer();

    TimerqueueElem *dummy_pkt_gen = snctx->dummy_traffic_timers[conf->conf_id];

    dummy_pkt_gen->arg = copy_conf;
    dummy_pkt_gen->callback = send_dummy_packet;
    dummy_pkt_gen->use_once = 0;
    dummy_pkt_gen->interval_us = conf->interval_ms * 1000;
    dummy_pkt_gen->detached_callback = dummypkt_send_job_detach_cb;
    dummy_pkt_gen->free_on_detach = 1;

    timerqueue_register_timer(snctx->timerqueue, dummy_pkt_gen);
}

void unregister_dummypkt_send_job(SimNetCtx *snctx,
                                  NetUnsetDummyTrafficConfig *conf)
{
    TLOGI("DUMMY STREAM ConfID:%u Stop\n", conf->conf_id);
    if (!snctx->dummy_traffic_timers[conf->conf_id]) {
        TLOGE("DUMMY STREAM ConfID:%u Not registered\n", conf->conf_id);
        return;
    }
    snctx->dummy_traffic_timers[conf->conf_id]->active = 0;
    snctx->dummy_traffic_timers[conf->conf_id] = NULL;
}
