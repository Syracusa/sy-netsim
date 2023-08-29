/**
 * @file simulator_config.c
 * @brief Parse json config to SimulatorConfig struct
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "cJSON.h"

#include "log.h"
#include "simulator_config.h"
#include "config_msg.h"

/** Read whole file and return as a buffer(newly allocated in function) */
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

void parse_config(SimulatorConfig *conf)
{
    char *config_string = read_file(SIMULATOR_CONFIG_FILE);
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
            printf("Node ID %d\n", nid);
            conf->active_node[nid] = 1;
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
            PhyLinkConfig *msg = &conf->linkconfs[conf->simlink_conf_num];
            msg->node_id_1 = cJSON_GetObjectItemCaseSensitive(link_json, "nid1")->valueint;
            msg->node_id_2 = cJSON_GetObjectItemCaseSensitive(link_json, "nid2")->valueint;

            cJSON *los_json = cJSON_GetObjectItemCaseSensitive(link_json, "los");
            if (cJSON_IsNumber(los_json)) {
                printf("Node %d <-> Node %d LOS : %d\n",
                       msg->node_id_1, msg->node_id_2, los_json->valueint);
                msg->los = los_json->valueint;
            } else {
                msg->los = 1; /* Default */
            }

            cJSON *pl_json = cJSON_GetObjectItemCaseSensitive(link_json, "pathloss");
            if (cJSON_IsNumber(pl_json)) {
                printf("Node %d <-> Node %d PATHLOSS : %lf\n",
                       msg->node_id_1, msg->node_id_2, pl_json->valuedouble);
                msg->pathloss_x100 = pl_json->valuedouble;
            } else {
                msg->pathloss_x100 = 0; /* Default */
            }
            conf->simlink_conf_num++;
        }
    }

    /* Dummy Traffic Config */
    cJSON *dummy_traffic_json = cJSON_GetObjectItemCaseSensitive(json, "dummy_traffic");
    if (cJSON_IsArray(dummy_traffic_json)) {
        cJSON *dummy_traffic_conf = NULL;
        cJSON_ArrayForEach(dummy_traffic_conf, dummy_traffic_json)
        {
            if (MAX_DUMMYSTREAM_CONF_NUM <= conf->dummystream_conf_num) {
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

            NetSetDummyTrafficConfig *stream_info =
                &(conf->dummy_stream_info[conf->dummystream_conf_num]);

            stream_info->src_id = src_id;
            stream_info->dst_id = dst_id;
            stream_info->payload_size = payload_size;
            stream_info->interval_ms = interval_ms;

            conf->dummystream_conf_num++;
        }
    }

    free(pretty_string);
    free(config_string);
    cJSON_Delete(json);
}