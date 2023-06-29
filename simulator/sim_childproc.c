#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h> 
#include <sys/prctl.h>

void start_net(int node_id)
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

void start_mac(int node_id)
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

void start_phy()
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

void start_simnode(int node_id)
{
    printf("Start simnode id %d\n", node_id);
    start_mac(node_id);
    start_net(node_id);
}