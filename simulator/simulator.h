#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "params.h"

typedef struct NodePositionGps{
    double latitude;
    double longitude;
    double altitude;
} NodePositionGps;

typedef struct SimLink{
    int los;
    double pathloss;
} SimLink;

typedef struct {
    int active;

    NodePositionGps pos;

    int mqid_net_command;
    int mqid_mac_command;

    int mqid_net_report;
    int mqid_mac_report;
} SimNode;

#define MAX_DUMMYSTREAM_NUM 100

typedef struct {
    int src_nid;
    int dst_nid;
    int payload_size;
    int interval_ms;
} DummyStreamInfo;

typedef struct {
    int dummy_stream_num;
    DummyStreamInfo dummy_stream_info[MAX_DUMMYSTREAM_NUM];
} SimulatorConfig;


typedef struct {
    int time_elapsed;

    int mqid_phy_command;
    int mqid_phy_report;

    SimulatorConfig conf;
    SimNode nodes[MAX_NODE_ID];
    SimLink links[MAX_NODE_ID][MAX_NODE_ID];
} SimulatorCtx;

#endif