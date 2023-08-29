/**
 * @file simulator_config.h
 * @brief Parse json config file and set simulator context
 */

#ifndef SIMULATOR_CONFIG_H
#define SIMULATOR_CONFIG_H

#include "sim_params.h"
#include "config_msg.h"

typedef struct SimulatorConfig {
    int active_node[MAX_NODE_ID]; /** 0-not activated, 1-activated */

    int dummystream_conf_num;
    NetSetDummyTrafficConfig dummy_stream_info[MAX_DUMMYSTREAM_CONF_NUM];

    int simlink_conf_num;
    PhyLinkConfig linkconfs[MAX_SIMLINK_CONF_NUM];
} SimulatorConfig;

/**
 * @brief Parse json config file(../config.json) and set simulator context
 * 
 * @param sctx Program context
 */
void parse_config(SimulatorConfig *conf);

#endif