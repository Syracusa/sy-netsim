
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <sys/types.h> 
#include <sys/prctl.h>

#include "log.h"

#include "simulator.h"
#include "simulator_config.h"

#include "params.h"
#include "mq.h"

#include "config_msg.h"

#include "httpserver.h"

char dbgname[10];
FILE* dbgfile;

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

    return sctx;
}

static void delete_simulator_context(SimulatorCtx *sctx)
{
    free(sctx);
}

static void start_net(int node_id)
{
    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Fork failed!\n");
    }
    if (pid == 0) {
        /* Kill child process when simulator die */
        int r = prctl(PR_SET_PDEATHSIG, SIGTERM);
        if (r == -1)
            fprintf(stderr, "prctl() failed!\n");
        printf("Simnet %d start with PID%d\n", node_id, (int)getpid());
        const char *file = "./bin/simnet";
        char nid_str[10];
        sprintf(nid_str, "%d", node_id);
        execl(file, file, nid_str, NULL);
    }
}

static void start_mac(int node_id)
{
    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Fork failed!\n");
    }
    if (pid == 0) {
        /* Kill child process when simulator die */
        int r = prctl(PR_SET_PDEATHSIG, SIGTERM);
        if (r == -1)
            fprintf(stderr, "prctl() failed!\n");
        printf("Simmac %d start with PID%d\n", node_id, (int)getpid());
        const char *file = "./bin/simmac";
        char nid_str[10];
        sprintf(nid_str, "%d", node_id);
        execl(file, file, nid_str, NULL);
    }
}

static void start_phy()
{
    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Fork failed!\n");
    }
    if (pid == 0) {
        /* Kill child process when simulator die */
        int r = prctl(PR_SET_PDEATHSIG, SIGTERM);
        if (r == -1)
            fprintf(stderr, "prctl() failed!\n");
        printf("Simphy start with PID%d\n", (int)getpid());
        const char *file = "./bin/simphy";
        execl(file, file, NULL);
    }
}

static void start_simnode(int node_id)
{
    printf("Start simnode id %d\n", node_id);
    start_mac(node_id);
    start_net(node_id);
}

static void start_simulate(SimulatorCtx *sctx)
{
    start_phy();

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
            start_simnode(i);
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
    MqMsgbuf mbuf;
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
    MqMsgbuf mbuf;
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

static void app_exit(int signo)
{
    if (g_sctx)
        delete_simulator_context(g_sctx);
}

int main()
{
    dbgfile = stderr;
    TLOGI("Start simulator...\n");

    SimulatorCtx *sctx = create_simulator_context();
    signal(SIGINT, &app_exit);

    parse_config(sctx);
    init_mq(sctx);

    init_http_server();

    start_simulate(sctx);
    sleep(1); /* Wait until apps are ready... */
    send_config_msgs(sctx);

    sleep(100);
    TLOGI("Finish\n");

    delete_simulator_context(sctx);
    return 0;
}
