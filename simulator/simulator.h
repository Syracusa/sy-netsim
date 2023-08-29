/**
 * @file simulator.h
 * @brief 
 */

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "sim_params.h"
#include "config_msg.h"
#include "report_msg.h"
#include "ringbuffer.h"
#include "pthread.h"
#include "simulator_config.h"

/** Information of one node */
typedef struct {
    /** Simulator => SimNet command message queue id */
    int mqid_net_command;

    /** Simulator => SimMAC command message queue id */
    int mqid_mac_command;

    /** SimNet => Simulator report message queue id */
    int mqid_net_report;

    /** SimMAC => Simulator report message queue id */
    int mqid_mac_report;

    /** Dummy mac process id */
    pid_t mac_pid;

    /** SimNet SW process id */
    pid_t net_pid;
} SimNode;

/** Simulator server context */
typedef struct SimulatorServerCtx {
    RingBuffer *recvq; /** TCP receive buffering queue */
    RingBuffer *sendq; /** TCP send buffering queue */
    int stop; /** if 1, stop TCP server */
    pthread_t tcp_thread; /** TCP server thread */
} SimulatorServerCtx;

/** Simulator program context */
typedef struct SimulatorCtx {
    int mqid_phy_command; /** Phy simulator message queue id(Simulator => SimPHY) */
    int mqid_phy_report; /** Phy simulator message queue id(SimPHY => Simulator) */
    pid_t phy_pid; /** Phy simulator process id */

    SimulatorConfig conf; /**  */
    SimNode nodes[MAX_NODE_ID];

    PhyLinkConfig link[MAX_NODE_ID][MAX_NODE_ID];
    SimulatorServerCtx server_ctx;
} SimulatorCtx;

/**
 * @brief 
 * Get the singleton simulator context.
 * If simulator context is not initialized, initialize it.
 * 
 * @return SimulatorCtx* Singleton simulator context
 */
SimulatorCtx *get_simulator_context();

/** Destroy singleton simulator context. */
void delete_simulator_context();

/** Kill all process that spawned by simulator */
void simulator_kill_all_process(SimulatorCtx *sctx);

/** Start simulator from config file */
void simulator_start_local(SimulatorCtx* sctx);

#endif