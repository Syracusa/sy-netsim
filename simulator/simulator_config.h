/**
 * @file simulator_config.h
 * @brief Parse json config file for simulator(only used in local mode).
 */

#ifndef SIMULATOR_CONFIG_H
#define SIMULATOR_CONFIG_H

#include "sim_params.h"
#include "config_msg.h"

/**
 * @brief Simulator configuration.
 * Fields are initiated from parsing json config file.
 * Include active node list, dummy stream list, dummy link info list.
 */
typedef struct SimulatorConfig {
    int active_node[MAX_NODE_ID]; /** 0-not activated, 1-activated */

    /** Number of dummy_stream_info */
    int dummystream_conf_num; 
    
    /** Configured dummy stream list */
    NetSetDummyTrafficConfig dummy_stream_info[MAX_DUMMYSTREAM_CONF_NUM];

    /** Number of link configuration */
    int simlink_conf_num;

    /** Configured dummy link info list */
    PhyLinkConfig linkconfs[MAX_SIMLINK_CONF_NUM];
} SimulatorConfig;

/**
 * @brief Parse json config file(SIMULATOR_CONFIG_FILE) and set simulator context
 * 
 * @param sctx Program context
 */
void parse_config(SimulatorConfig *conf);

#endif