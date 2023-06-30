#ifndef SIM_CHILDPROC_H
#define SIM_CHILDPROC_H

#include "simulator.h"

void start_simnode(SimulatorCtx* sctx, int node_id);
pid_t start_net(int node_id);
pid_t start_mac(int node_id);
pid_t start_phy();

#endif