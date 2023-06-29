#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "params.h"
#include "config_msg.h"
#include "ringbuffer.h"

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
} SimNode;

#define MAX_DUMMYSTREAM_CONF_NUM 100

#define MAX_SIMLINK_CONF_NUM 1000
typedef struct {
    int dummystream_conf_num;
    NetDummyTrafficConfig dummy_stream_info[MAX_DUMMYSTREAM_CONF_NUM];

    int simlink_conf_num;
    PhyLinkConfig linkconfs[MAX_SIMLINK_CONF_NUM];
} SimulatorConfig;

typedef struct {
    RingBuffer *recvq;
    RingBuffer *sendq;
    int stop;
} SimulatorServerCtx;

typedef struct {
    int time_elapsed;

    int mqid_phy_command;
    int mqid_phy_report;

    SimulatorConfig conf;
    SimNode nodes[MAX_NODE_ID];

    SimulatorServerCtx server_ctx;
} SimulatorCtx;

#endif