#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "simnet.h"
#include "dummy.h"
#include "mac_connection.h"
#include "net_report.h"
#include "net_command.h"

SimNetCtx *g_snctx = NULL; /* Global singleton simnet context */

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
