#include "mac_connection.h"

void sendto_mac(SimNetCtx *snctx, void *data, size_t len, long type)
{
    send_mq(snctx->mqid_send_mac, data, len, type);
}

void process_mac_msg(SimNetCtx *snctx, void *data, int len)
{
    struct iphdr *iph = data;

    if (DEBUG_NET_TRX) {
        if (PKT_HEXDUMP)
            hexdump(data, len, stdout);

        TLOGD("Recv pkt. %s <- %s(Payloadsz: %ld)\n",
              ip2str(iph->daddr), ip2str(iph->saddr), len - IPUDP_HDRLEN);
    }

    snctx->route->handle_remote_pkt(data, len);
}

void recvfrom_mac(SimNetCtx *snctx)
{
    MqMsgbuf msg;
    while (1) {
        ssize_t res = recv_mq(snctx->mqid_recv_mac, &msg);
        if (res < 0) {
            break;
        }

        switch (msg.type) {
            case MESSAGE_TYPE_DATA:
                process_mac_msg(snctx, msg.text, res);
                break;
            default:
                TLOGE("Unknown msgtype %ld\n", msg.type);
                break;
        }
    }
}
