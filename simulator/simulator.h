#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "params.h"

typedef struct {
    int active;

    int mqid_net_command;
    int mqid_mac_command;

    int mqid_net_report;
    int mqid_mac_report;

} Simnode;

typedef struct {
    int time_elapsed;

    int mqid_phy_command;
    int mqid_phy_report;
    
    Simnode nodes[MAX_NODE_ID];
} SimulatorCtx;

#endif