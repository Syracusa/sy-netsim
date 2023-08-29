#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h> 
#include <sys/prctl.h>

#include "simulator.h"

/** Execute child binarys(simnet/mac/phy) */
static pid_t execute_simulation_component_binary(char *bin_path, int node_id)
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

        if (node_id < 0) {
            printf("%s start with PID%d\n", bin_path, (int)getpid());
            execl(bin_path, bin_path, NULL);
        } else {
            char nid_str[10];
            sprintf(nid_str, "%d", node_id);
            printf("%s with id %d start with PID%d\n", bin_path, node_id, (int)getpid());
            execl(bin_path, bin_path, nid_str, NULL);
        }
    }
    return pid;
}

/** Execute simnet */
static pid_t start_net(int node_id)
{
    return execute_simulation_component_binary(SIMNET_BINARY_PATH, node_id);
}

/** Execute simmac */
static pid_t start_mac(int node_id)
{
    return execute_simulation_component_binary(SIMMAC_BINARY_PATH, node_id);
}

pid_t start_simphy()
{
    return execute_simulation_component_binary(SIMPHY_BINARY_PATH, -1);
}

void start_simnode(SimulatorCtx *sctx, int node_id)
{
    printf("Start simnode id %d\n", node_id);
    sctx->nodes[node_id].mac_pid = start_mac(node_id);
    sctx->nodes[node_id].net_pid = start_net(node_id);
}