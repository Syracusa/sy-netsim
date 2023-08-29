#include <stdio.h>
#include <signal.h>
#include <netinet/in.h>

#include "mq.h"
#include "log.h"
#include "cJSON.h"
#include "sim_childproc.h"
#include "sim_remote_conf.h"

#define REMOTE_CONF_VERBOSE 0

static void start_simulate_remote(SimulatorCtx *sctx, int nodenum)
{
    simulator_kill_all_process(sctx);

    sctx->phy_pid = start_phy();
    for (int i = 0; i < nodenum; i++) {
        start_simnode(sctx, i);
    }
}

static void handle_start_simulation_msg(SimulatorCtx *sctx, cJSON *json)
{
    cJSON *type = cJSON_GetObjectItem(json, "nodenum");
    if (cJSON_IsNumber(type)) {
        int nodenum = type->valueint;
        TLOGD("nodenum: %d\n", nodenum);

        start_simulate_remote(sctx, nodenum);
    } else {
        TLOGE("Can't find nodenum in json\n");
    }
}

static void handle_link_info_msg(SimulatorCtx *sctx, cJSON *json)
{
    cJSON *type = cJSON_GetObjectItem(json, "links");
    PhyLinkConfig msg;
    if (cJSON_IsArray(type)) {
        cJSON *links = NULL;
        int node1 = 0;
        cJSON_ArrayForEach(links, type)
        {
            int node2 = node1 + 1;
            cJSON *link = NULL;
            cJSON_ArrayForEach(link, links)
            {
                PhyLinkConfig *old = &sctx->link[node1][node2];
                if (cJSON_IsNumber(link)) {
                    double linkval = link->valuedouble;

                    if (REMOTE_CONF_VERBOSE)
                        TLOGD("link %d <-> %d : %lf\n", node1, node2, linkval);

                    msg.node_id_1 = node1;
                    msg.node_id_2 = node2;
                    if (linkval > 1000.0) {
                        msg.los = 0;
                    } else {
                        msg.los = 1;
                    }
                    msg.pathloss_x100 = 0;

                    if (old->los != msg.los ||
                        old->pathloss_x100 != msg.pathloss_x100) {
                        old->los = msg.los;
                        old->pathloss_x100 = msg.pathloss_x100;
                    }
                }

                send_mq(sctx->mqid_phy_command,
                        &msg, sizeof(msg), CONF_MSG_TYPE_PHY_LINK_CONFIG);

                if (REMOTE_CONF_VERBOSE)
                    TLOGD("UPDATE %d <=> %d\n", node1, node2);

                node2++;
            }
            node1++;
        }
    }
}

static void handle_start_dummy_traffic(SimulatorCtx *sctx, cJSON *json, int is_update)
{
    int conf_id;

    cJSON *confIdJson = cJSON_GetObjectItem(json, "confId");
    if (cJSON_IsNumber(confIdJson)) {
        conf_id = confIdJson->valueint;
    } else {
        TLOGE("Can't find confId in json\n");
        return;
    }

    NetSetDummyTrafficConfig *msg = &(sctx->conf.dummy_stream_info[conf_id]);
    msg->conf_id = conf_id;

    cJSON *senderJson = cJSON_GetObjectItem(json, "sourceNodeId");
    if (cJSON_IsNumber(senderJson)) {
        msg->src_id = senderJson->valueint;
    } else {
        TLOGE("Can't find sender in json\n");
        return;
    }

    cJSON *receiverJson = cJSON_GetObjectItem(json, "destinationNodeId");
    if (cJSON_IsNumber(receiverJson)) {
        msg->dst_id = receiverJson->valueint;
    } else {
        TLOGE("Can't find receiver in json\n");
        return;
    }

    cJSON *payloadJson = cJSON_GetObjectItem(json, "packetSize");
    if (cJSON_IsNumber(payloadJson)) {
        msg->payload_size = payloadJson->valueint;
    } else {
        TLOGE("Can't find payload in json\n");
        return;
    }

    cJSON *intervalJson = cJSON_GetObjectItem(json, "intervalMs");
    if (cJSON_IsNumber(intervalJson)) {
        msg->interval_ms = intervalJson->valueint;
    } else {
        TLOGE("Can't find interval in json\n");
        return;
    }

    int code = CONF_MSG_TYPE_NET_SET_DUMMY_TRAFFIC;
    if (is_update)
        code = CONF_MSG_TYPE_NET_UPDATE_DUMMY_TRAFFIC;

    send_mq(sctx->nodes[msg->src_id].mqid_net_command,
            msg, sizeof(NetSetDummyTrafficConfig), code);
}

static void handle_stop_dummy_traffic(SimulatorCtx *sctx, cJSON *json)
{
    NetUnsetDummyTrafficConfig msg;
    cJSON *confIdJson = cJSON_GetObjectItem(json, "confId");
    if (cJSON_IsNumber(confIdJson)) {
        msg.conf_id = confIdJson->valueint;
    } else {
        TLOGE("Can't find confId in json\n");
        return;
    }

    NetSetDummyTrafficConfig *stream_info =
        &(sctx->conf.dummy_stream_info[msg.conf_id]);

    send_mq(sctx->nodes[stream_info->src_id].mqid_net_command,
            &msg, sizeof(NetUnsetDummyTrafficConfig),
            CONF_MSG_TYPE_NET_UNSET_DUMMY_TRAFFIC);
}

void handle_remote_conf_msg(SimulatorCtx *sctx, char *jsonstr)
{
    /* Get type from json */
    cJSON *json = cJSON_Parse(jsonstr);
    cJSON *type = cJSON_GetObjectItem(json, "type");
    if (type == NULL) {
        TLOGE("Can't find type in json(%s)\n", jsonstr);
        goto out;
    }

    if (cJSON_IsString(type)) {
        char *typestr = type->valuestring;

        if (REMOTE_CONF_VERBOSE)
            TLOGD("type: %s\n", typestr);

        if (strcmp(typestr, "Status") == 0) {
            char buf[1000];
            sprintf(buf, "{\"type\":\"Status\",\"status\":\"OK\"}");
            uint16_t len = strlen(buf);
            len = htons(len);
            RingBuffer_push(sctx->server_ctx.sendq, &len, 2);
            RingBuffer_push(sctx->server_ctx.sendq, buf, strlen(buf));
        } else if (strcmp(typestr, "Start") == 0) {
            handle_start_simulation_msg(sctx, json);
        } else if (strcmp(typestr, "LinkInfo") == 0) {
            handle_link_info_msg(sctx, json);
        } else if (strcmp(typestr, "NewDummyTrafficConf") == 0) {
            /* Do nothing */
            TLOGE("NewDummyTrafficConf\n");
        } else if (strcmp(typestr, "DeleteDummyTrafficConf") == 0) {
            handle_stop_dummy_traffic(sctx, json);
            TLOGE("DeleteDummyTrafficConf\n");
        } else if (strcmp(typestr, "UpdateDummyTrafficConf") == 0) {
            handle_start_dummy_traffic(sctx, json, 1);
            TLOGE("UpdateDummyTrafficConf\n");
        } else if (strcmp(typestr, "StartDummyTraffic") == 0) {
            handle_start_dummy_traffic(sctx, json, 0);
            TLOGE("StartDummyTraffic\n");
        } else if (strcmp(typestr, "StopDummyTraffic") == 0) {
            handle_stop_dummy_traffic(sctx, json);
            TLOGE("StopDummyTraffic\n");
        } else {
            TLOGE("Unknown type: %s\n", typestr);
        }
    }

out:
    cJSON_Delete(json);
}
