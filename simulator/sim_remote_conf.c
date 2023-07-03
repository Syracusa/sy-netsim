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
        kill(sctx->phy_pid, SIGKILL);
        killnum++;
        sctx->phy_pid = 0;
    }

    for (int i = 0; i < MAX_NODE_ID; i++) {
        if (sctx->nodes[i].net_pid > 0) {
            kill(sctx->nodes[i].net_pid, SIGKILL);
            killnum++;
            sctx->nodes[i].net_pid = 0;
        }
        if (sctx->nodes[i].mac_pid > 0) {
            kill(sctx->nodes[i].mac_pid, SIGKILL);
            killnum++;
            sctx->nodes[i].mac_pid = 0;
        }
    }

    TLOGD("%d processes terminated\n", killnum);
}

static void start_simulate_remote(SimulatorCtx *sctx, int nodenum)
{
    kill_all_process(sctx);

    start_phy();
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
                PhyLinkConfig* old = &sctx->link[node1][node2];
                if (cJSON_IsNumber(link)) {
                    double linkval = link->valuedouble;
                    // TLOGD("link %d <-> %d : %lf\n",
                    //       node1, node2, linkval);
                    msg.node_id_1 = node1;
                    msg.node_id_2 = node2;
                    if (linkval > 15.0){
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
                
                // TLOGD("UPDATE %d <=> %d\n", node1, node2);

                node2++;
            }
            node1++;
        }
    }
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
        // TLOGD("type: %s\n", typestr);
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
        } else {
            TLOGE("Unknown type: %s\n", typestr);
        }
    }

out:
    cJSON_Delete(json);
}
