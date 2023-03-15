#include <stdio.h>
#include <stdint.h>

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


int main()
{
    printf("SIMNET STARTED\n");
    SimPhyContext spctx;

    /* TODO : Read config from file */

    init_mac_connection_context(&spctx);
}