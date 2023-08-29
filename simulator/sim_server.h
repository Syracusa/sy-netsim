#ifndef SIM_SERVER_H
#define SIM_SERVER_H

#include "simulator.h"

/** Start TCP server to recv conf & send report */
void simulator_start_server(SimulatorServerCtx *ssctx);

/** Stop TCP server */
void simulator_stop_server(SimulatorServerCtx *ssctx);

/** Mainloop. Receive report, send configuration, and so on */
void simulator_server_mainloop(SimulatorCtx *sctx);

#endif