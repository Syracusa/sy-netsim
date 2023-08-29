/**
 * @file simulator_config.h
 * @brief Parse json config file and set simulator context
 */

#ifndef SIMULATOR_CONFIG_H
#define SIMULAOTR_CONFIG_H

#include "simulator.h"


/**
 * @brief Parse json config file(../config.json) and set simulator context
 * 
 * @param sctx Program context
 */
void parse_config(SimulatorCtx *sctx);

#endif