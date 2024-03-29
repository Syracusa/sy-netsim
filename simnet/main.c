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

static void simnet_init_mq(SimNetCtx *snctx)
{
    /* NET => MAC Message queue init */
    int mqkey_send_mac = MQ_KEY_NET_TO_MAC + snctx->node_id;
    snctx->mqid_send_mac = msgget(mqkey_send_mac, IPC_CREAT | 0666);

    /* MAC => NET Message queue init */
    int mqkey_recv_mac = MQ_KEY_MAC_TO_NET + snctx->node_id;
    snctx->mqid_recv_mac = msgget(mqkey_recv_mac, IPC_CREAT | 0666);

    /* NET => Simulator Message queue init */
    int mqkey_send_report = MQ_KEY_NET_REPORT + snctx->node_id;
    snctx->mqid_send_report = msgget(mqkey_send_report, IPC_CREAT | 0666);

    /* Simulator => NET Message queue init */
    int mqkey_recv_command = MQ_KEY_NET_COMMAND + snctx->node_id;
    snctx->mqid_recv_command = msgget(mqkey_recv_command, IPC_CREAT | 0666);

    /* Flush old messages */
    mq_flush(snctx->mqid_send_mac);
    mq_flush(snctx->mqid_recv_mac);
    mq_flush(snctx->mqid_send_report);
    mq_flush(snctx->mqid_recv_command);
}

int main(int argc, char *argv[])
{
    SimNetCtx *snctx = get_simnet_context();
    parse_arg(snctx, argc, argv);

    /* Initiate message queue */
    simnet_init_mq(snctx);

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