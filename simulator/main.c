#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h> /* waitpid() */

#include "simulator.h"
#include "sim_server.h"
#include "log.h"

extern int flag_mainloop_exit;
char dbgname[10];
FILE *dbgfile;

static void app_exit(int signo)
{
    flag_mainloop_exit = 1;
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
        delete_simulator_context();
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


int main()
{
    dbgfile = stderr;
    TLOGI("Start simulator...\n");

    SimulatorCtx* sctx = get_simulator_context();

    handle_signal();

    int SERVER_MODE = 1;
    if (SERVER_MODE) {
        simulator_start_server(&sctx->server_ctx);
        simulator_mainloop(sctx);
    } else {
        simulator_start_local(sctx);
    }

    TLOGI("Finish\n");
    sleep(3600);
    delete_simulator_context();
    return 0;
}
