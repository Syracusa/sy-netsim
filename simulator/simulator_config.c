#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "cJSON.h"

#include "log.h"
#include "simulator_config.h"

static char *read_file(const char *filename)
{
    FILE *f = fopen(filename, "r");

    if (!f){
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

static void activate_node(SimulatorCtx* sctx, int node_id)
{
    if (node_id < 0 && node_id >= MAX_NODE_ID) {
        fprintf(stderr, "Unavailable node id : %d\n", node_id);
        exit(2);
    }
    sctx->nodes[node_id].active = 1;
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

    cJSON *node_list_json = cJSON_GetObjectItemCaseSensitive(json, "nodes");
    if (cJSON_IsArray(node_list_json)) {
        cJSON *node_id_json = NULL;
        cJSON_ArrayForEach(node_id_json, node_list_json)
        {
            if (cJSON_IsNumber(node_id_json)) {
                LOGD("Node ID : %d\n", node_id_json->valueint);
            } else {
                fprintf(stderr, "Node id is not a number!\n");
                exit(2);
            }
        }
    } else {
        fprintf(stderr, "Can't get nodelist!");
        exit(2);
    }

    free(pretty_string);
    free(config_string);
    cJSON_Delete(json);
}