#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/wait.h>
#include <sys/types.h> 
#include <netinet/in.h>

#include "log.h"
#include "params.h"
#include "mq.h"
#include "config_msg.h"

#include "simulator.h"
#include "simulator_config.h"
#include "sim_server.h"

#include "sim_childproc.h"
#include "sim_hdlreport.h"

#include "cJSON.h"

/** Global simulator context */
SimulatorCtx *g_sctx = NULL;

void simulator_init_mq(SimulatorCtx *sctx)
{
    for (int nid = 0; nid < MAX_NODE_ID; nid++) {
        int mqkey_net_command = MQ_KEY_NET_COMMAND + nid;
        sctx->nodes[nid].mqid_net_command = msgget(mqkey_net_command, IPC_CREAT | 0666);
        mq_flush(sctx->nodes[nid].mqid_net_command);

        int mqkey_net_report = MQ_KEY_NET_REPORT + nid;
        sctx->nodes[nid].mqid_net_report = msgget(mqkey_net_report, IPC_CREAT | 0666);
        mq_flush(sctx->nodes[nid].mqid_net_report);

        int mqkey_mac_command = MQ_KEY_MAC_COMMAND + nid;
        sctx->nodes[nid].mqid_mac_command = msgget(mqkey_mac_command, IPC_CREAT | 0666);
        mq_flush(sctx->nodes[nid].mqid_mac_command);

        int mqkey_mac_report = MQ_KEY_MAC_REPORT + nid;
        sctx->nodes[nid].mqid_mac_report = msgget(mqkey_mac_report, IPC_CREAT | 0666);
        mq_flush(sctx->nodes[nid].mqid_mac_report);
    }

    int mqkey_phy_command = MQ_KEY_PHY_COMMAND;
    sctx->mqid_phy_command = msgget(mqkey_phy_command, IPC_CREAT | 0666);
    mq_flush(sctx->mqid_phy_command);

    int mqkey_phy_report = MQ_KEY_PHY_REPORT;
    sctx->mqid_phy_report = msgget(mqkey_phy_report, IPC_CREAT | 0666);
    mq_flush(sctx->mqid_phy_report);
}

static SimulatorCtx *initialize_simulator_context()
{
    SimulatorCtx *sctx = malloc(sizeof(SimulatorCtx));
    memset(sctx, 0x00, sizeof(SimulatorCtx));

    for (int i = 0; i < MAX_NODE_ID; i++) {
        for (int j = 0; j < MAX_NODE_ID; j++) {
            PhyLinkConfig *link = &sctx->link[i][j];
            link->node_id_1 = i;
            link->node_id_2 = j;
            link->los = 1;
            link->pathloss_x100 = 0;
        }
    }
    simulator_init_mq(sctx);
    return sctx;
}

SimulatorCtx *get_simulator_context()
{
    if (g_sctx == NULL) {
        g_sctx = initialize_simulator_context();
    }
    return g_sctx;
}

void free_simulator_context()
{
    if (g_sctx) {
        simulator_free_server_buffers(&g_sctx->server_ctx);
        free(g_sctx);
    }
    g_sctx = NULL;
}

static void start_simulate_local(SimulatorCtx *sctx)
{
    sctx->phy_pid = start_simphy();

    for (int i = 0; i < MAX_NODE_ID; i++) {
        if (sctx->conf.active_node[i] == 1) {
            start_simnode(sctx, i);
        }
    }
}

static void send_dummystream_config_msgs(SimulatorCtx *sctx)
{
    SimulatorConfig *conf = &(sctx->conf);

    /* Dummy stream config */
    for (int i = 0; i < conf->dummystream_conf_num; i++) {
        NetSetDummyTrafficConfig *info = &(conf->dummy_stream_info[i]);

        int res = send_mq(sctx->nodes[info->src_id].mqid_net_command,
                          info, sizeof(NetSetDummyTrafficConfig),
                          CONF_MSG_TYPE_NET_SET_DUMMY_TRAFFIC);
        if (res < 0) {
            TLOGE("Failed to send dummy stream config to NET\n");
        }
    }
}

static void send_link_config_msgs(SimulatorCtx *sctx)
{
    for (int i = 0; i < sctx->conf.simlink_conf_num; i++) {
        int res = send_mq(sctx->mqid_phy_command,
                          &sctx->conf.linkconfs[i], sizeof(PhyLinkConfig),
                          CONF_MSG_TYPE_PHY_LINK_CONFIG);
        if (res < 0) {
            TLOGE("Failed to send link config to PHY\n");
        }
    }
}

static void send_config_msgs(SimulatorCtx *sctx)
{
    send_dummystream_config_msgs(sctx);
    send_link_config_msgs(sctx);
}

void simulator_kill_all_process(SimulatorCtx *sctx)
{
    int killnum = 0;

    if (sctx->phy_pid > 0) {
        kill(sctx->phy_pid, SIGKILL);
        killnum++;
        sctx->phy_pid = 0;
    }

    for (int i = 0; i < MAX_NODE_ID; i++) {
        if (sctx->nodes[i].net_pid > 0) {
            kill(sctx->nodes[i].net_pid, SIGKILL);
            killnum++;
            sctx->nodes[i].net_pid = 0;
        }
        if (sctx->nodes[i].mac_pid > 0) {
            kill(sctx->nodes[i].mac_pid, SIGKILL);
            killnum++;
            sctx->nodes[i].mac_pid = 0;
        }
    }

    TLOGD("%d processes terminated\n", killnum);
}

void simulator_start_local(SimulatorCtx *sctx, const char *filepath)
{
    parse_config(&sctx->conf, filepath);
    start_simulate_local(sctx);

    /* Wait until apps are ready.
     Message queues will be flushed when each apps initiate themselves.
     So we shouldn't send config before the apps complete initiating */
    sleep(1);

    send_config_msgs(sctx);
}

void simulator_local_mainloop(SimulatorCtx *sctx)
{
    while (1) {
        recv_report(sctx);
        usleep(10 * 1000);
    }
}