#include <stdio.h>

#include <netinet/in.h>

#include "log.h"
#include "cJSON.h"
#include "sim_childproc.h"
#include "sim_remote_conf.h"

static void start_simulate_remote(SimulatorCtx *sctx, int nodenum)
{
    start_phy();

    for (int i = 0; i < nodenum; i++) {
        start_simnode(i);
    }
}

static void handle_start_simulation_msg(SimulatorCtx *sctx, cJSON *json)
{
    cJSON *type = cJSON_GetObjectItem(json, "nodenum");
    if (cJSON_IsNumber(type)){
        int nodenum = type->valueint;
        TLOGD("nodenum: %d\n", nodenum);

        start_simulate_remote(sctx, nodenum);
    } else {
        TLOGE("Can't find nodenum in json\n");
    }
}

void handle_remote_conf_msg(SimulatorCtx *sctx, char *jsonstr)
{
    /* Get type from json */
    cJSON *json = cJSON_Parse(jsonstr);
    cJSON *type = cJSON_GetObjectItem(json, "type");
    if (type == NULL) {
        TLOGE("Can't find type in json\n");
        return;
    }
    if (cJSON_IsString(type)){
        char *typestr = type->valuestring;
        TLOGD("type: %s\n", typestr);
        if (strcmp(typestr, "Status") == 0){
            char buf[1000];
            sprintf(buf, "{\"type\":\"Status\",\"status\":\"OK\"}");
            uint16_t len = strlen(buf);
            len = htons(len);
            RingBuffer_push(sctx->server_ctx.sendq, &len, 2);
            RingBuffer_push(sctx->server_ctx.sendq, buf, strlen(buf));
        } else if (strcmp(typestr, "Start") == 0){
            handle_start_simulation_msg(sctx, json);
        } else {
            TLOGE("Unknown type: %s\n", typestr);
        }
    }
}
