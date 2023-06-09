#include "mac_connection.h"

void sendto_mac(SimNetCtx *snctx, void *data, size_t len, long type)
{
    /* The mtype field must have a strictly positive integer value. */
    if (type < 1) {
        TLOGE("Can't send meg with type %ld\n", type);
        return;
    }

    MqMsgbuf msg;
    msg.type = type;

    if (len > MQ_MAX_DATA_LEN) {
        TLOGE("Can't send data with length %lu\n", len);
    }
    memcpy(msg.text, data, len);
    int ret = msgsnd(snctx->mqid_send_mac, &msg, len, IPC_NOWAIT);
    if (ret < 0) {
        if (errno == EAGAIN) {
            TLOGE("Message queue full!\n");
        } else {
            TLOGE("Can't send to mac. mqid: %d len: %lu(%s)\n",
                  snctx->mqid_send_mac, len, strerror(errno));
        }
    }
}

void process_mac_msg(SimNetCtx *snctx, void *data, int len)
{
    PktBuf pkb;
    memcpy(&pkb.iph, data, len);
    pkb.payload_len = len - IPUDP_HDRLEN;

    if (PKT_HEXDUMP)
        hexdump(data, len, stdout);

    TLOGD("Recv pkt. %s <- %s(Payloadsz: %ld)\n",
          ip2str(pkb.iph.daddr), ip2str(pkb.iph.saddr), pkb.payload_len);
}

void recvfrom_mac(SimNetCtx *snctx)
{
    MqMsgbuf msg;
    while (1) {
        ssize_t res = msgrcv(snctx->mqid_recv_mac, &msg, sizeof(msg.text), 0, IPC_NOWAIT);
        if (res < 0) {
            if (errno != ENOMSG) {
                TLOGE("Msgrcv failed(err: %s)\n", strerror(errno));
            }
            break;
        }

        switch (msg.type)
        {
        case MESSAGE_TYPE_DATA:
            process_mac_msg(snctx, msg.text, res);
            break;
        default:
            TLOGE("Unknown msgtype %ld\n", msg.type);
            break;
        }
            
    }
}
