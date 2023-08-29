/**
 * @file sim_server.h
 * @brief Connect frontend with TCP server
 */

#ifndef SIM_SERVER_H
#define SIM_SERVER_H

#include "simulator.h"

/** Start TCP server to recv conf & send report */
void simulator_start_server(SimulatorServerCtx *ssctx);

/** Stop TCP server */
void simulator_free_server_buffers(SimulatorServerCtx *ssctx);

/** Server mode mainloop. Receive report, send configuration, and so on */
void simulator_server_mainloop(SimulatorCtx *sctx);

#endif