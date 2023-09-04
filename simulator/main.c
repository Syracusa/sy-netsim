#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h> /* waitpid() */

#include "simulator.h"
#include "sim_server.h"
#include "log.h"

extern int g_flag_server_mainloop_exit;
char dbgname[10];
FILE *dbgfile;

static void app_exit(int signo)
{
    g_flag_server_mainloop_exit = 1;
    static int reenter = 0;

    if (reenter)
        return;
    reenter = 1;

    TLOGF("SIGINT\n");

    SimulatorCtx* sctx = get_simulator_context();

    if (sctx) {
        simulator_kill_all_process(sctx);
        sctx->server_ctx.stop = 1;
        pthread_join(sctx->server_ctx.tcp_thread, NULL);
        free_simulator_context();
        TLOGF("Exit simulator...\n");
        exit(SIGKILL);
    } else {
        TLOGF("Context is null\n");
    }
}

static void handle_sigchld(int signo)
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


int main(int argc, char *argv[])
{
    dbgfile = stderr;
    TLOGI("Start simulator...\n");

    SimulatorCtx* sctx = get_simulator_context();

    handle_signal();

    if (argc == 2){
        /* Config file option - run as local mode */
        sctx->server_mode = 0;
        simulator_start_local(sctx, argv[1]);
        simulator_local_mainloop(sctx);
    } else if (argc == 1) {
        /* No config file option - run as server mode */
        sctx->server_mode = 1;
        simulator_start_server(&sctx->server_ctx);
        simulator_server_mainloop(sctx);
    } else {
        TLOGF("Usage: %s [config_file]\n", argv[0]);
        exit(2);
    }

    TLOGI("Finish\n");
    sleep(3600);
    free_simulator_context();
    return 0;
}
