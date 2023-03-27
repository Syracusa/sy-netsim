#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "params.h"

typedef struct {
    int active;
} Simnode;

typedef struct {
    int time_elapsed;

    Simnode nodes[MAX_NODE_ID];
} SimulatorCtx;

#endif