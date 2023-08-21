#include <stdio.h>
#include <signal.h>
#include <netinet/in.h>

#include "log.h"
#include "cJSON.h"
#include "sim_childproc.h"
#include "sim_remote_conf.h"

static void kill_all_process(SimulatorCtx *sctx)
{
    /* Kill all processes before start */

    int killnum = 0;

    if (sctx->phy_pid > 0) {
        kill(sctx->phy_pid, SIGINT);
        killnum++;
        sctx->phy_pid = 0;
    }

    for (int i = 0; i < MAX_NODE_ID; i++) {
        if (sctx->nodes[i].net_pid > 0) {
            kill(sctx->nodes[i].net_pid, SIGINT);
            killnum++;
            sctx->nodes[i].net_pid = 0;
        }
        if (sctx->nodes[i].mac_pid > 0) {
            kill(sctx->nodes[i].mac_pid, SIGINT);
            killnum++;
            sctx->nodes[i].mac_pid = 0;
        }
    }

    TLOGD("%d processes terminated\n", killnum);
}

static void start_simulate_remote(SimulatorCtx *sctx, int nodenum)
{
    kill_all_process(sctx);

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
#if 0
                    TLOGD("link %d <-> %d : %lf\n", node1, node2, linkval);
#endif
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

                send_config(sctx, sctx->mqid_phy_command,
                            &msg, sizeof(msg), CONF_MSG_TYPE_PHY_LINK_CONFIG);

#if 0
                TLOGD("UPDATE %d <=> %d\n", node1, node2);
#endif
                node2++;
            }
            node1++;
        }
    }
}

static void handle_traffic_control_msg(SimulatorCtx *sctx, cJSON *json)
{
    NetDummyTrafficConfig msg;
    cJSON *senderJson = cJSON_GetObjectItem(json, "sender");
    if (cJSON_IsNumber(senderJson)) {
        msg.src_id = senderJson->valueint;
    } else {
        TLOGE("Can't find sender in json\n");
        return;
    }

    cJSON *receiverJson = cJSON_GetObjectItem(json, "receiver");
    if (cJSON_IsNumber(receiverJson)) {
        msg.dst_id = receiverJson->valueint;
    } else {
        TLOGE("Can't find receiver in json\n");
        return;
    }

    cJSON *payloadJson = cJSON_GetObjectItem(json, "payload");
    if (cJSON_IsNumber(payloadJson)) {
        msg.payload_size = payloadJson->valueint;
    } else {
        TLOGE("Can't find payload in json\n");
        return;
    }

    cJSON *intervalJson = cJSON_GetObjectItem(json, "interval");
    if (cJSON_IsNumber(intervalJson)) {
        msg.interval_ms = intervalJson->valueint;
    } else {
        TLOGE("Can't find interval in json\n");
        return;
    }

    send_config(sctx, sctx->nodes[msg.src_id].mqid_net_command,
                &msg, sizeof(NetDummyTrafficConfig),
                CONF_MSG_TYPE_NET_DUMMY_TRAFFIC);
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

#if 0
        TLOGD("type: %s\n", typestr);
#endif
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
        } else if (strcmp(typestr, "TrafficControl")) {
            handle_traffic_control_msg(sctx, json);
        } else {
            TLOGE("Unknown type: %s\n", typestr);
        }
    }

out:
    cJSON_Delete(json);
}
