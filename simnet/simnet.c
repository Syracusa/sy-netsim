#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "simnet.h"
#include "dummy.h"
#include "mac_connection.h"
#include "net_report.h"
#include "net_command.h"

SimNetCtx *g_snctx = NULL; /* Global singleton simnet context */

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

/**
 * @brief Actual works of simnet
 * 
 * @param snctx Simnet context
 */
static void simnet_work(SimNetCtx *snctx)
{
    /* Receive and handle messages from MAC and Simulator */
    recvfrom_mac(snctx);
    recv_command(snctx);

    /* Fire timer events */
    timerqueue_work(snctx->timerqueue);

    /* Let route module do its work */
    snctx->route->work();
}

void simnet_mainloop(SimNetCtx *snctx)
{
    /* This function executes simnet_work() every 1ms */
    struct timespec before, after, diff, req, rem;

    while (1) {
        clock_gettime(CLOCK_REALTIME, &before);

        simnet_work(snctx); /* Do actual work */

        clock_gettime(CLOCK_REALTIME, &after);
        timespec_sub(&after, &before, &diff);
        if (diff.tv_nsec < 1000 * 1000) {
            req.tv_sec = 0;
            req.tv_nsec = 1000 * 1000 - diff.tv_nsec;
            nanosleep(&req, &rem);
        }
    }
}

SimNetCtx *get_simnet_context()
{
    if (g_snctx == NULL) {
        SimNetCtx *snctx = malloc(sizeof(SimNetCtx));
        memset(snctx, 0x00, sizeof(SimNetCtx));

        snctx->timerqueue = create_timerqueue();

        /* Initiate message queue */
        simnet_init_mq(snctx);

        g_snctx = snctx;
    }
    return g_snctx;
}

void delete_simnet_context(SimNetCtx *snctx)
{
    free(snctx);
}

void remote_send(void *data, size_t len)
{
    sendto_mac(g_snctx, data, len, 1);
}

void local_send(void *data, size_t len)
{
    struct iphdr *iph = (struct iphdr *)data;
    TLOGD("Send to local(%s => %s)\n", ip2str(iph->saddr), ip2str(iph->daddr));
}
