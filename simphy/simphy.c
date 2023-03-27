#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "simphy.h"

#include "log.h"
#include "params.h"


static void send_to_remote_mac(SimPhyCtx *spctx,
                               uint8_t id,
                               void *data, size_t len)
{
    /* TODO : Multicast */
}

static void recv_from_remote_mac(SimPhyCtx *spctx,
                                 uint8_t id,
                                 void *data,
                                 size_t len)
{
    /* TODO : Multicast Recv */
}

static void send_to_local_mac(SimPhyCtx *spctx,
                              uint8_t id,
                              void *data,
                              size_t len)
{
    /* TODO : Message queue */
}

static void recv_from_local_mac(SimPhyCtx *spctx,
                                uint8_t id,
                                void *data,
                                size_t len)
{
    for (int i = 0; i < MAX_NODE_ID; i++) {
        MacConnCtx *mac_conn = &(spctx->node[i].macconn);
        if (mac_conn->alive) {
            send_to_local_mac(spctx, i, data, len);
        }
    }
}

static SimPhyCtx *create_context()
{
    SimPhyCtx *spctx = malloc(sizeof(SimPhyCtx));
    memset(spctx, 0x00, sizeof(SimPhyCtx));

    for (int i = 0; i < MAX_NODE_ID; i++) {
        spctx->node[i].node_id = i;
    }
    return spctx;
}

static void delete_context(SimPhyCtx *spctx)
{
    free(spctx);
}

int main()
{
    printf("SIMPHY STARTED\n");
    SimPhyCtx *spctx = create_context();


    delete_context(spctx);
}