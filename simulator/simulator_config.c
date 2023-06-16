#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "cJSON.h"

#include "log.h"
#include "simulator_config.h"

static char *read_file(const char *filename)
{
    FILE *f = fopen(filename, "r");

    if (!f) {
        fprintf(stderr, "Can't open file %s\n", filename);
        exit(2);
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET); /* same as rewind(f); */

    char *string = malloc(fsize + 1);
    fread(string, fsize, 1, f);
    fclose(f);

    string[fsize] = 0;

    return string;
}

static void activate_node(SimulatorCtx *sctx, int node_id,
                          double lat, double lon, double alt)
{
    if (node_id < 0 && node_id >= MAX_NODE_ID) {
        fprintf(stderr, "Unavailable node id : %d\n", node_id);
        exit(2);
    }
    SimNode *node = &sctx->nodes[node_id];
    node->active = 1;
    node->pos.altitude = alt;
    node->pos.latitude = lat;
    node->pos.longitude = lon;
}

void parse_config(SimulatorCtx *sctx)
{
    char *config_string = read_file("../config.json");
    cJSON *json = cJSON_Parse(config_string);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        fprintf(stderr, "Config file parse error!\n");
        if (error_ptr != NULL)
            fprintf(stderr, "Error before: %s\n", error_ptr);
        exit(2);
    }

    char *pretty_string = cJSON_Print(json);
    printf("%s\n", pretty_string);

    /* Node ID List */
    cJSON *node_list_json = cJSON_GetObjectItemCaseSensitive(json, "nodes");
    if (cJSON_IsArray(node_list_json)) {
        cJSON *node_json = NULL;
        cJSON_ArrayForEach(node_json, node_list_json)
        {
            int nid = cJSON_GetObjectItemCaseSensitive(node_json, "id")->valueint;
            cJSON *nodepos_json = cJSON_GetObjectItemCaseSensitive(node_json, "pos");

            double lat = 0.0;
            double lon = 0.0;
            double alt = 0.0;

            if (node_json) {
                lat = cJSON_GetObjectItemCaseSensitive(nodepos_json, "lat")->valuedouble;
                lon = cJSON_GetObjectItemCaseSensitive(nodepos_json, "lon")->valuedouble;
                alt = cJSON_GetObjectItemCaseSensitive(nodepos_json, "alt")->valuedouble;
            }

            printf("Node ID %d  lat %lf lon %lf alt %lf\n", nid, lat, lon, alt);
            activate_node(sctx, nid, lat, lon, alt);
        }
    } else {
        fprintf(stderr, "Can't get nodelist!");
        exit(2);
    }

    /* Link List */
    cJSON *link_list_json = cJSON_GetObjectItemCaseSensitive(json, "links");
    if (cJSON_IsArray(link_list_json)) {
        cJSON *link_json = NULL;
        cJSON_ArrayForEach(link_json, link_list_json)
        {
            PhyLinkConfig* msg = &sctx->conf.linkconfs[sctx->conf.simlink_conf_num];
            int nid1 = cJSON_GetObjectItemCaseSensitive(link_json, "nid1")->valueint;
            int nid2 = cJSON_GetObjectItemCaseSensitive(link_json, "nid2")->valueint;

            cJSON *los_json = cJSON_GetObjectItemCaseSensitive(link_json, "los");
            if (cJSON_IsNumber(los_json)) {
                printf("Node %d <-> Node %d LOS : %d\n", nid1, nid2, los_json->valueint);
                msg->los = los_json->valueint;
            } else {
                msg->los = 1; /* Default */
            }

            cJSON *pl_json = cJSON_GetObjectItemCaseSensitive(link_json, "pathloss");
            if (cJSON_IsNumber(pl_json)) {
                printf("Node %d <-> Node %d PATHLOSS : %lf\n", nid1, nid2, pl_json->valuedouble);
                msg->pathloss_x100 = pl_json->valuedouble;
            } else {
                msg->pathloss_x100 = 0; /* Default */
            }
            sctx->conf.simlink_conf_num++;
        }
    }

    /* Dummy Traffic Config */
    cJSON *dummy_traffic_json = cJSON_GetObjectItemCaseSensitive(json, "dummy_traffic");
    if (cJSON_IsArray(dummy_traffic_json)) {
        cJSON *dummy_traffic_conf = NULL;
        cJSON_ArrayForEach(dummy_traffic_conf, dummy_traffic_json)
        {
            if (MAX_DUMMYSTREAM_CONF_NUM <= sctx->conf.dummystream_conf_num) {
                fprintf(stderr, "Dummystream number overflow! max : %d\n",
                        MAX_DUMMYSTREAM_CONF_NUM);
                exit(2);
            }

            int src_id = cJSON_GetObjectItemCaseSensitive(dummy_traffic_conf, "src_id")->valueint;
            int dst_id = cJSON_GetObjectItemCaseSensitive(dummy_traffic_conf, "dst_id")->valueint;
            int payload_size = cJSON_GetObjectItemCaseSensitive(dummy_traffic_conf, "payload_size")->valueint;
            int interval_ms = cJSON_GetObjectItemCaseSensitive(dummy_traffic_conf, "interval_ms")->valueint;

            TLOGI("Dummy traffic %d -> %d  payload %dbyte  interval %dms\n",
                  src_id, dst_id, payload_size, interval_ms);

            /*
            At this point, No netproto apps are excuted.
            So we can't send config message right now.
            */

            NetDummyTrafficConfig *stream_info =
                &(sctx->conf.dummy_stream_info[sctx->conf.dummystream_conf_num]);

            stream_info->src_id = src_id;
            stream_info->dst_id = dst_id;
            stream_info->payload_size = payload_size;
            stream_info->interval_ms = interval_ms;

            sctx->conf.dummystream_conf_num++;
        }
    }

    free(pretty_string);
    free(config_string);
    cJSON_Delete(json);
}