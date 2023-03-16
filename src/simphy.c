#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

#define MAX_NODE_ID 128

typedef struct
{
    uint8_t id;
    uint8_t alive;
} MacConnCtx;

typedef struct
{
    MacConnCtx mac_conn[MAX_NODE_ID];
} SimPhyContext;

static void init_mac_connection_context(SimPhyContext *spctx)
{
    for (int i = 0; i < MAX_NODE_ID; i++)
    {
        MacConnCtx *mac_conn = &(spctx->mac_conn[i]);
        mac_conn->id = i;
        mac_conn->alive = 0;
    }
}

static void send_to_remote_mac(SimPhyContext *spctx,
                               uint8_t id,
                               void *data, size_t len)
{
    /* TODO : Multicast */
}

static void recv_from_remote_mac(SimPhyContext *spctx,
                                 uint8_t id,
                                 void *data,
                                 size_t len)
{
    /* TODO : Multicast Recv */
}

static void send_to_local_mac(SimPhyContext *spctx,
                              uint8_t id,
                              void *data,
                              size_t len)
{
    /* TODO : Message queue */
}

static void recv_from_local_mac(SimPhyContext *spctx,
                                uint8_t id,
                                void *data,
                                size_t len)
{
    for (int i = 0; i < MAX_NODE_ID; i++)
    {
        MacConnCtx *mac_conn = &(spctx->mac_conn[i]);
        if (mac_conn->alive)
        {
            send_to_local_mac(spctx, mac_conn->id, data, len);
        }
    }
}

static char *read_file(const char *filename)
{
    FILE *f = fopen(filename, "r");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET); /* same as rewind(f); */

    char *string = malloc(fsize + 1);
    fread(string, fsize, 1, f);
    fclose(f);

    string[fsize] = 0;

    return string;
}

static void parse_config(SimPhyContext *spctx)
{
    char *config_string = read_file("../config.json");
    cJSON *json = cJSON_Parse(config_string);
    if (json == NULL){
        const char *error_ptr = cJSON_GetErrorPtr();
        fprintf(stderr, "Config file parse error!\n");
        if (error_ptr != NULL)
            fprintf(stderr, "Error before: %s\n", error_ptr);
        exit(2);
    }

    char *pretty_string = cJSON_Print(json);
    printf("%s\n", pretty_string);

    free(pretty_string);
    free(config_string);
    cJSON_Delete(json);
}

static SimPhyContext *create_context()
{
    SimPhyContext* spctx = malloc(sizeof(SimPhyContext));
    memset(spctx, 0x00, sizeof(SimPhyContext));

    return spctx;
}

static void delete_context(SimPhyContext* spctx)
{
    free(spctx);
}

int main()
{
    printf("SIMNET STARTED\n");
    SimPhyContext *spctx = create_context();

    /* TODO : Read config from file */
    parse_config(spctx);
    init_mac_connection_context(spctx);

    delete_context(spctx);
}