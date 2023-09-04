#include "net_command.h"
#include "dummy.h"

static void process_command(SimNetCtx *snctx, void *data, int len, long type)
{
    switch (type) {
        case CONF_MSG_TYPE_NET_SET_DUMMY_TRAFFIC:
            TLOGI("CONF_MSG_TYPE_NET_SET_DUMMY_TRAFFIC Length : %d\n", len);
            register_dummypkt_send_job(snctx, data);
            break;
        case CONF_MSG_TYPE_NET_UNSET_DUMMY_TRAFFIC:
            TLOGI("CONF_MSG_TYPE_NET_UNSET_DUMMY_TRAFFIC Length : %d\n", len);
            unregister_dummypkt_send_job(snctx, data);
            break;
        case CONF_MSG_TYPE_NET_UPDATE_DUMMY_TRAFFIC:
            TLOGI("CONF_MSG_TYPE_NET_UPDATE_DUMMY_TRAFFIC Length : %d\n", len);
            unregister_dummypkt_send_job(snctx, data);
            register_dummypkt_send_job(snctx, data);
            break;
        default:
            TLOGI("Unknown command %ld received. Length : %d\n", type, len);
            break;
    }
}

void recv_command(SimNetCtx *snctx)
{
    MqMsgbuf msg;
    while (1) {
        ssize_t res = recv_mq(snctx->mqid_recv_command, &msg);
        if (res < 0)
            break;
        
        process_command(snctx, msg.text, res, msg.type);
    }
}