#include <stdio.h>
#include <sys/stat.h> // mkdir()

#include "simnet.h"
#include "net_report.h"

#include "olsr/olsr_route_iface.h"

char dbgname[10];
FILE *dbgfile;

static void parse_arg(SimNetCtx *snctx, int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage : ./%s {NODE_ID}\n", argv[0]);
        exit(2);
    }

    int res = sscanf(argv[1], "%u", &(snctx->node_id));

    if (res < 1) {
        fprintf(stderr, "Can't parse nodeID from [%s]\n", argv[1]);
        exit(2);
    }
}

static void init_log(SimNetCtx *snctx)
{
    mkdir("/tmp/viewlog", 0777);
    
    sprintf(dbgname, "NET-%-2d", snctx->node_id);
    dbgfile = stderr;

    char logfile[100];
    sprintf(logfile, "/tmp/viewlog/n%d", snctx->node_id);
    FILE *f = fopen(logfile, "w+");
    if (f)
        dbgfile = f;
}

int main(int argc, char *argv[])
{
    SimNetCtx *snctx = get_simnet_context();

    parse_arg(snctx, argc, argv);
    printf("Simnet start with nodeid %d\n", snctx->node_id);
    srand(time(NULL) + snctx->node_id);

    init_log(snctx);

    /* Set routing context */
    MAKE_BE32_IP(myip, 192, 168, snctx->node_id, 1);
    CommonRouteConfig rconf = {
        .own_ip = myip,
        .send_remote = remote_send,
        .send_local = local_send,
        .stat = &snctx->stat
    };
    snctx->route = &olsr_iface;
    snctx->route->start(&rconf);

    start_report_job(snctx);

    simnet_mainloop(snctx);

    snctx->route->end();
    delete_simnet_context(snctx);
    return 0;
}