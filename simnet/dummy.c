#include "simnet.h"
#include "mac_connection.h"

static void send_dummy_packet(void *arg)
{
    NetDummyTrafficConfig *conf = arg;
    SimNetCtx *snctx = g_snctx;
    static uint8_t dummybuf[MAX_IPPKT_SIZE];

    PktBuf pkb;
    memset(&pkb, 0x00, sizeof(PktBuf));

    pkb.payload_len = conf->payload_size;

    MAKE_BE32_IP(sender_ip, 192, 168, snctx->node_id, 1);
    MAKE_BE32_IP(receiver_ip, 192, 168, conf->dst_id, 1);

    build_ip_hdr(&(pkb.iph), pkb.payload_len + IPUDP_HDRLEN, 64,
                 sender_ip, receiver_ip, IPPROTO_UDP);
    pkb.iph_len = sizeof(struct iphdr);
    build_udp_hdr_no_checksum(&(pkb.udph), 29111, 29112, pkb.payload_len);

    if (DEBUG_NET_TRX) {
        TLOGD("Send dummy pkt. %s -> %s(Payloadsz: %ld)\n",
              ip2str(pkb.iph.saddr), ip2str(pkb.iph.daddr), pkb.payload_len);

        if (PKT_HEXDUMP)
            hexdump(&pkb.iph, pkb.payload_len + IPUDP_HDRLEN, stdout);
    }
    size_t len = MAX_IPPKT_SIZE;
    ippkt_pack(&pkb, dummybuf, &len);
    snctx->route->handle_local_pkt(dummybuf, len);
}

static void dummypkt_send_job_detach_cb(void *arg)
{
    NetDummyTrafficConfig *conf = arg;
    free(conf);
}

void register_dummypkt_send_job(SimNetCtx *snctx,
                                NetDummyTrafficConfig *conf)
{
    NetDummyTrafficConfig *copy_conf = malloc(sizeof(NetDummyTrafficConfig));
    memcpy(copy_conf, conf, sizeof(NetDummyTrafficConfig));

    TLOGI("DUMMY STREAM Dst:%u, Src: %u, Interval: %u, Payload size: %u\n",
          conf->dst_id, conf->src_id, conf->interval_ms, conf->payload_size);

    TimerqueueElem *dummy_pkt_gen = timerqueue_new_timer();

    dummy_pkt_gen->arg = copy_conf;
    dummy_pkt_gen->callback = send_dummy_packet;
    dummy_pkt_gen->use_once = 0;
    dummy_pkt_gen->interval_us = conf->interval_ms * 1000;
    dummy_pkt_gen->detached_callback = dummypkt_send_job_detach_cb;
    dummy_pkt_gen->free_on_detach = 1;

    timerqueue_register_timer(snctx->timerqueue, dummy_pkt_gen);
}
