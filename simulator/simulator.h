#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "params.h"
#include "config_msg.h"
#include "report_msg.h"
#include "ringbuffer.h"
#include "pthread.h"

typedef struct NodePositionGps {
    double latitude;
    double longitude;
    double altitude;
} NodePositionGps;

typedef struct {
    int active;

    NodePositionGps pos;

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

typedef struct SimulatorCtx {
    int started;
    int time_elapsed;

    int mqid_phy_command;
    int mqid_phy_report;

    pid_t phy_pid;

    SimulatorConfig conf;
    SimNode nodes[MAX_NODE_ID];

    PhyLinkConfig link[MAX_NODE_ID][MAX_NODE_ID];
    SimulatorServerCtx server_ctx;
} SimulatorCtx;

void send_config(SimulatorCtx *sctx,
                 int mqid,
                 void *data,
                 size_t len,
                 long type);

#endif