#ifndef SIMPHY_H
#define SIMPHY_H

#include <stdint.h>
#include "params.h"


typedef struct
{
    uint8_t alive;
} MacConnCtx;

typedef struct 
{
    int node_id;
    MacConnCtx macconn;
} NodeCtx;

typedef struct
{
    NodeCtx node[MAX_NODE_ID];
} SimPhyCtx;

#endif