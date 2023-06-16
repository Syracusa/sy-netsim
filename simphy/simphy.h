#ifndef SIMPHY_H
#define SIMPHY_H

#include <stdint.h>
#include "params.h"

typedef struct SimLink{
    int los;
    double pathloss;
} SimLink;

typedef struct 
{
    int alive;

    int mqid_send_mac;
    int mqid_recv_mac;
} NodeCtx;

typedef struct
{
    NodeCtx nodes[MAX_NODE_ID];
    int mqid_recv_command;
    int mqid_send_report;
    SimLink links[MAX_NODE_ID][MAX_NODE_ID];

} SimPhyCtx;

#endif