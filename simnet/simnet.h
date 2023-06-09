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

#include "log.h"
#include "route_iface.h"

typedef struct
{
    TqCtx *timerqueue;
    int node_id;

    int mqid_recv_mac;
    int mqid_send_mac;

    int mqid_recv_command;
    int mqid_send_report;

    RouteContext *route;
} SimNetCtx;


extern SimNetCtx *g_snctx;


#endif