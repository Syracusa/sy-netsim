/**
 * @file simnet.h
 * @brief Network layer simulator
 */

#ifndef SIMNET_H
#define SIMNET_H

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <errno.h>

#include "mq.h"
#include "util.h"
#include "timerqueue.h"
#include "netutil.h"

#include "params.h"

#include "config_msg.h"
#include "report_msg.h"

#include "log.h"
#include "route_iface.h"
#include "net_statistics.h"

#define MAX_DUMMY_STREAMS 1000 /* Global dummy traffic id max */

typedef struct
{
    TimerqueueContext *timerqueue;
    int node_id;

    int mqid_recv_mac;
    int mqid_send_mac;

    int mqid_recv_command;
    int mqid_send_report;

    RouteFunctions *route;
    TimerqueueElem *dummy_traffic_timers[MAX_DUMMY_STREAMS];

    NetStats stat;
} SimNetCtx;


extern SimNetCtx *g_snctx;


#endif