/**
 * @file sim_remote_conf.h
 * @brief Handle remote configuration message from frontend
 */

#ifndef SIM_REMOTE_CONF_H
#define SIM_REMOTE_CONF_H

#include "simulator.h"

/** Handle JSON msg received from frontend */
void handle_remote_conf_msg(SimulatorCtx *sctx, char *jsonstr);

#endif