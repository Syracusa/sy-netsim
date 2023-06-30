#include <unistd.h>

#include "simnet.h"
#include "dummy.h"
#include "mac_connection.h"

#include "olsr/olsr_route_iface.h"

SimNetCtx *g_snctx = NULL;
char dbgname[10];
FILE *dbgfile;

void init_mq(SimNetCtx *snctx)
{
    int mqkey_send_mac = MQ_KEY_NET_TO_MAC + snctx->node_id;
    snctx->mqid_send_mac = msgget(mqkey_send_mac, IPC_CREAT | 0666);

    int mqkey_recv_mac = MQ_KEY_MAC_TO_NET + snctx->node_id;
    snctx->mqid_recv_mac = msgget(mqkey_recv_mac, IPC_CREAT | 0666);

    int mqkey_send_report = MQ_KEY_NET_REPORT + snctx->node_id;
    snctx->mqid_send_report = msgget(mqkey_send_report, IPC_CREAT | 0666);

    int mqkey_recv_command = MQ_KEY_NET_COMMAND + snctx->node_id;
    snctx->mqid_recv_command = msgget(mqkey_recv_command, IPC_CREAT | 0666);

    mq_flush(snctx->mqid_send_mac);
    mq_flush(snctx->mqid_recv_mac);

    mq_flush(snctx->mqid_send_report);
    mq_flush(snctx->mqid_recv_command);
}

static void process_command(SimNetCtx *snctx, void *data, int len, long type)
{
    switch (type) {
        case CONF_MSG_TYPE_NET_DUMMY_TRAFFIC:
            TLOGI("CONF_MSG_TYPE_NET_DUMMY_TRAFFIC Length : %d\n", len);
            register_dummypkt_send_job(snctx, data);
            break;
        default:
            TLOGI("Unknown command %ld received. Length : %d\n", type, len);
            break;
    }
}

void recv_command(SimNetCtx *snctx)
{
    MqMsgbuf msg;
    while (1) {
        ssize_t res = msgrcv(snctx->mqid_recv_command,
                             &msg, sizeof(msg.text), 0, IPC_NOWAIT);
        if (res < 0) {
            if (errno != ENOMSG) {
                TLOGE("Msgrcv failed(err: %s)\n", strerror(errno));
            }
            break;
        }
        process_command(snctx, msg.text, res, msg.type);
    }
}

void mainloop(SimNetCtx *snctx)
{
    struct timespec before, after, diff, req, rem;

    while (1) {
        clock_gettime(CLOCK_REALTIME, &before);

        timerqueue_work(snctx->timerqueue);
        recvfrom_mac(snctx);
        recv_command(snctx);

        snctx->route->work();

        clock_gettime(CLOCK_REALTIME, &after);
        timespec_sub(&after, &before, &diff);
        if (diff.tv_nsec < 1000 * 1000) {
            req.tv_sec = 0;
            req.tv_nsec = 1000 * 1000 - diff.tv_nsec;
            nanosleep(&req, &rem);
        }
    }
}

static int rbtree_compare_by_inetaddr(const void *k1,
                                      const void *k2)
{
    in_addr_t *n1 = (in_addr_t *)k1;
    in_addr_t *n2 = (in_addr_t *)k2;

    return *n1 - *n2;
}

static SimNetCtx *create_simnet_context()
{
    SimNetCtx *snctx = malloc(sizeof(SimNetCtx));
    memset(snctx, 0x00, sizeof(SimNetCtx));

    snctx->timerqueue = create_timerqueue();
    
    return snctx;
}

SimNetCtx *get_simnet_context()
{
    return g_snctx;
}

static void delete_simnet_context(SimNetCtx *snctx)
{
    free(snctx);
}

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

static void send_dummy_log_to_simulator(void *arg)
{
    // fprintf(stderr, "TODO : Send dummy log to simulator\n");
}

static void remote_send(void *data, size_t len)
{
    sendto_mac(g_snctx, data, len, 1);
}

static void local_send(void *data, size_t len)
{
    /* TODO */
}

int main(int argc, char *argv[])
{
    SimNetCtx *snctx = create_simnet_context();
    g_snctx = snctx;
    parse_arg(snctx, argc, argv);
    printf("Simnet start with nodeid %d\n", snctx->node_id);
    srand(time(NULL) + snctx->node_id);

    sprintf(dbgname, "NET-%-2d", snctx->node_id);
    dbgfile = stderr;

    char logfile[100];
    sprintf(logfile, "/tmp/viewlog/n%d", snctx->node_id);
    FILE *f = fopen(logfile, "w+");
    if (f)
        dbgfile = f;

    /* Initiate message queue */
    init_mq(snctx);

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

    mainloop(snctx);

    snctx->route->end();
    delete_simnet_context(snctx);
    return 0;
}