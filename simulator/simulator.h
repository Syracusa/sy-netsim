/**
 * @file simulator.h
 * @brief 
 */

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "params.h"
#include "config_msg.h"
#include "report_msg.h"
#include "ringbuffer.h"
#include "pthread.h"

/** Information of one node */
typedef struct {
    int active;

    int mqid_net_command;
    int mqid_mac_command;

    int mqid_net_report;
    int mqid_mac_report;

    pid_t mac_pid;
    pid_t net_pid;
} SimNode;

#define MAX_DUMMYSTREAM_CONF_NUM 100
#define MAX_SIMLINK_CONF_NUM 1000

typedef struct SimulatorConfig {
    int dummystream_conf_num;
    NetSetDummyTrafficConfig dummy_stream_info[MAX_DUMMYSTREAM_CONF_NUM];

    int simlink_conf_num;
    PhyLinkConfig linkconfs[MAX_SIMLINK_CONF_NUM];
} SimulatorConfig;

typedef struct SimulatorServerCtx {
    RingBuffer *recvq;
    RingBuffer *sendq;
    int stop;

    pthread_t tcp_thread;
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


/** */
void simulator_start_local(SimulatorCtx* sctx);

/** */
void send_config(SimulatorCtx *sctx,
                 int mqid,
                 void *data,
                 size_t len,
                 long type);

#endif