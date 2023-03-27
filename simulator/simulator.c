#include <sys/prctl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"

#include "simulator.h"
#include "simulator_config.h"

#include "params.h"

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

    start_simulate(sctx);
    sleep(10);

    delete_simulator_context(sctx);
    return 0;
}
