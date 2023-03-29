
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h> 
#include <sys/prctl.h>

#include "log.h"

#include "simulator.h"
#include "simulator_config.h"

#include "params.h"
#include "mq.h"

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
    if (pid == -1){
        fprintf(stderr, "Fork failed!\n");
    }
    if (pid == 0) {
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
    if (pid == -1){
        fprintf(stderr, "Fork failed!\n");
    }
    if (pid == 0) {
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
    if (pid == -1){
        fprintf(stderr, "Fork failed!\n");
    }
    if (pid == 0) {
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
        if (sctx->nodes[i].active == 1) {
            start_simnode(i);
        }
    }
}

int main()
{
    TLOGI("Start simulator...\n");
    SimulatorCtx *sctx = create_simulator_context();

    parse_config(sctx);
    init_mq(sctx);

    start_simulate(sctx);
    sleep(10);

    delete_simulator_context(sctx);
    return 0;
}
