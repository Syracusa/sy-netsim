#ifndef SIMPHY_H
#define SIMPHY_H

#include <stdint.h>
#include "params.h"

typedef struct 
{
    int alive;

    int mqid_send_mac;
    int mqid_recv_mac;
} NodeCtx;

typedef struct
{
    NodeCtx nodes[MAX_NODE_ID];
    
} SimPhyCtx;

#endif