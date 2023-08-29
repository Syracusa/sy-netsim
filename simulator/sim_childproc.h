/**
 * @file sim_childproc.h
 * @brief Execute simnet/simmac/simphy as child process
 */

#ifndef SIM_CHILDPROC_H
#define SIM_CHILDPROC_H

#include "simulator.h"

/**
 * @brief Start simnet and simmac. 
 * Simphy is shared between nodes so not executed in this function
*/
void start_simnode(SimulatorCtx* sctx, int node_id);

/** Start phy simulator */
pid_t start_simphy();

#endif