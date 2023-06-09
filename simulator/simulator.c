
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
#include "sim_remote_conf.h"
#include "sim_childproc.h"
#include "sim_hdlreport.h"

#include "cJSON.h"

char dbgname[10];
FILE *dbgfile;

SimulatorCtx *g_sctx = NULL;

#define D2R(d) (d / 180.0 * 3.14159)

double calc_node_distance(NodePositionGps *p1, NodePositionGps *p2)
{
    double earth_radius = 6371.0;
    double x1 = (earth_radius + D2R(p1->altitude)) * cos(D2R(p1->latitude)) * cos(D2R(p1->longitude));
    double y1 = (earth_radius + D2R(p1->altitude)) * cos(D2R(p1->latitude)) * sin(D2R(p1->longitude));
    double z1 = (earth_radius + D2R(p1->altitude)) * sin(D2R(p1->latitude));
    double x2 = (earth_radius + D2R(p2->altitude)) * cos(D2R(p2->latitude)) * cos(D2R(p2->longitude));
    double y2 = (earth_radius + D2R(p2->altitude)) * cos(D2R(p2->latitude)) * sin(D2R(p2->longitude));
    double z2 = (earth_radius + D2R(p2->altitude)) * sin(D2R(p2->latitude));

    double xdiff = x2 - x1;
    double ydiff = y2 - y1;
    double zdiff = z2 - z1;

    return sqrt(xdiff * xdiff + ydiff * ydiff + zdiff * zdiff);
}

void init_mq(SimulatorCtx *sctx)
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

static SimulatorCtx *create_simulator_context()
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

    return sctx;
}

static void delete_simulator_context()
{
    if (g_sctx) {
        server_end(&g_sctx->server_ctx);
        free(g_sctx);
    }
    g_sctx = NULL;
}


static void start_simulate_local(SimulatorCtx *sctx)
{
    sctx->phy_pid = start_phy();

    for (int i = 0; i < MAX_NODE_ID; i++) {
        for (int j = i + 1; j < MAX_NODE_ID; j++) {
            SimNode *n1 = &sctx->nodes[i];
            SimNode *n2 = &sctx->nodes[j];
            if (n1->active != 1)
                continue;
            if (n2->active != 1)
                continue;

            double dist = calc_node_distance(&n1->pos, &n2->pos);
            TLOGD("Dist between %d and %d  ==>  %lf\n", i, j, dist);
        }
    }

    for (int i = 0; i < MAX_NODE_ID; i++) {
        if (sctx->nodes[i].active == 1) {
            start_simnode(sctx, i);
        }
    }
}

void send_config(SimulatorCtx *sctx,
                 int mqid,
                 void *data,
                 size_t len,
                 long type)
{
    MqMsgbuf msg;
    msg.type = type;

    if (len > MQ_MAX_DATA_LEN) {
        TLOGE("Can't send data with length %lu\n", len);
    }
    memcpy(msg.text, data, len);
    int ret = msgsnd(mqid, &msg, len, IPC_NOWAIT);
    if (ret < 0) {
        if (errno == EAGAIN) {
            TLOGE("Message queue full!\n");
        } else {
            TLOGE("Can't send config. mqid: %d len: %lu(%s)\n",
                  mqid, len, strerror(errno));
        }
    }
}

static void send_dummystream_config_msgs(SimulatorCtx *sctx)
{
    SimulatorConfig *conf = &(sctx->conf);

    /* Dummy stream config */
    for (int i = 0; i < conf->dummystream_conf_num; i++) {
        NetDummyTrafficConfig *info = &(conf->dummy_stream_info[i]);

        send_config(sctx, sctx->nodes[info->src_id].mqid_net_command,
                    info, sizeof(NetDummyTrafficConfig),
                    CONF_MSG_TYPE_NET_DUMMY_TRAFFIC);
    }
}

static void send_link_config_msgs(SimulatorCtx *sctx)
{
    for (int i = 0; i < sctx->conf.simlink_conf_num; i++) {
        send_config(sctx, sctx->mqid_phy_command,
                    &sctx->conf.linkconfs[i], sizeof(PhyLinkConfig),
                    CONF_MSG_TYPE_PHY_LINK_CONFIG);
    }
}

static void send_config_msgs(SimulatorCtx *sctx)
{
    send_dummystream_config_msgs(sctx);
    send_link_config_msgs(sctx);
}

void parse_client_json(SimulatorServerCtx *ssctx)
{
    if (!ssctx->recvq)
        return;

    while (1) {
        size_t canread = RingBuffer_get_readable_bufsize(ssctx->recvq);
        uint16_t jsonlen;
        if (canread >= 2) {
            RingBuffer_read(ssctx->recvq, &jsonlen, 2);
            jsonlen = ntohs(jsonlen);

            uint16_t tmp;
            if (canread >= 2 + jsonlen) {
                RingBuffer_pop(ssctx->recvq, &tmp, 2);
                char *jsonstr = malloc(jsonlen + 1);
                RingBuffer_pop(ssctx->recvq, jsonstr, jsonlen);
                jsonstr[jsonlen] = '\0';
                handle_remote_conf_msg(g_sctx, jsonstr);
                free(jsonstr);
            } else {
                break;
            }
        } else {
            break;
        }
    }
}


int g_exit = 0;

static void kill_all_process(SimulatorCtx *sctx)
{
    /* Kill all processes before start */

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

void app_exit(int signo)
{
    g_exit = 1;
    static int reenter = 0;

    if (reenter)
        return;
    reenter = 1;

    TLOGF("SIGINT\n");

    if (g_sctx) {
        kill_all_process(g_sctx);
        g_sctx->server_ctx.stop = 1;
        pthread_join(g_sctx->server_ctx.tcp_thread, NULL);
        delete_simulator_context();
        TLOGF("Exit simulator...\n");
        exit(SIGKILL);
    } else {
        TLOGF("Context is null\n");
    }
}

static void mainloop(SimulatorCtx *sctx)
{
    while (!g_exit) {
        parse_client_json(&sctx->server_ctx);
        recv_report(sctx);
        usleep(10 * 1000);
    }
}

void handle_sigchld(int signo)
{
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        TLOGD("Child process %d terminated\n", pid);
    }
}

static void handle_signal()
{
    signal(SIGINT, &app_exit);
    if (HANDLE_SIGCHLD)
        signal(SIGCHLD, handle_sigchld);
}

static void start_local(SimulatorCtx* sctx)
{
    parse_config(sctx);
    start_simulate_local(sctx);
    sleep(1); /* Wait until apps are ready... */
    send_config_msgs(sctx);
}

int main()
{
    dbgfile = stderr;
    TLOGI("Start simulator...\n");

    SimulatorCtx *sctx = create_simulator_context();
    g_sctx = sctx;

    handle_signal();

    init_mq(sctx);

    int SERVER_MODE = 1;
    if (SERVER_MODE) {
        start_server(&sctx->server_ctx);
        mainloop(sctx);
    } else {
        start_local(sctx);
    }

    TLOGI("Finish\n");
    sleep(3600);
    delete_simulator_context();
    return 0;
}
