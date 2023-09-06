#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>

#include "simphy.h"

#include "log.h"
#include "params.h"
#include "mq.h"
#include "util.h"

#include "config_msg.h"
#include "report_msg.h"

char dbgname[10];
FILE *dbgfile;

/**
 * @brief Send data to MAC
 *
 * @param spctx Simphy context
 * @param receiver_nid Receiver node id
 * @param data Actual data
 * @param len Length of data
 * @param type Message type
 */
static void send_to_mac(SimPhyCtx *spctx,
                        int receiver_nid,
                        void *data,
                        size_t len,
                        long type)
{
    send_mq(spctx->nodes[receiver_nid].mqid_send_mac, data, len, type);
}

/**
 * @brief Process MAC frame and forward to other nodes
 *
 * @param spctx Simphy context
 * @param sender_nid Sender node id
 * @param data Actual data
 * @param len Length of data
 */
static void process_mac_frame(SimPhyCtx *spctx,
                              int sender_nid,
                              void *data,
                              size_t len)
{
    for (int nid = 0; nid < MAX_NODE_ID; nid++) {
        if (sender_nid == nid)
            continue;

        if (spctx->nodes[nid].alive == 1) {
            if (DEBUG_MAC_TRX)
                TLOGI("PHY msg forwarding... %d -> %d\n", sender_nid, nid);

            SimLink *link = &spctx->links[sender_nid][nid];
            if (link->los != 1) {
                if (DEBUG_PHY_DROP)
                    TLOGW("PHY DROP FRAME %d => %d LOS\n", sender_nid, nid);
                continue;
            }
            send_to_mac(spctx, nid, data, len, MESSAGE_TYPE_DATA);
        }
    }
}

/**
 * @brief Poll messages from MAC
 *
 * @param spctx Simphy context
 */
static void recv_from_mac(SimPhyCtx *spctx)
{
    for (int nid = 0; nid < MAX_NODE_ID; nid++) {
        MqMsgbuf msg;
        while (1) {
            ssize_t res = recv_mq(spctx->nodes[nid].mqid_recv_mac, &msg);
            if (res < 0)
                break;

            spctx->nodes[nid].alive = 1;

            switch (msg.type) {
                case MESSAGE_TYPE_DATA:
                    process_mac_frame(spctx, nid, msg.text, res);
                    break;
                case MESSAGE_TYPE_HEARTBEAT:
                    TLOGE("Heartbeat recvd from MAC %d\n", nid);
                    break;
                default:
                    TLOGE("Unknown msgtype %ld\n", msg.type);
                    break;
            }
        }
    }
}

/**
 * @brief Parse link config command and set link state
 *
 * @param spctx Simphy context
 * @param msg PhyLinkConfig message
 */
static void handle_link_config_command(SimPhyCtx *spctx,
                                       void *msg)
{
    PhyLinkConfig *lmsg = msg;

    SimLink *l1 = &spctx->links[lmsg->node_id_1][lmsg->node_id_2];
    SimLink *l2 = &spctx->links[lmsg->node_id_2][lmsg->node_id_1];

    l1->los = l2->los = lmsg->los;
    l1->pathloss_x100 = l2->pathloss_x100 = lmsg->pathloss_x100;

    if (DEBUG_LINK)
        TLOGI("Link %u <=> %u  LOS %u  PASSLOSS %u\n",
              lmsg->node_id_1, lmsg->node_id_2,
              lmsg->los, lmsg->pathloss_x100);
}

/**
 * @brief Handle command from simulator
 *
 * @param spctx spctx Simphy context
 */
static void recv_command(SimPhyCtx *spctx)
{
    MqMsgbuf msg;
    while (1) {
        ssize_t res = recv_mq(spctx->mqid_recv_command, &msg);
        if (res < 0)
            break;

        switch (msg.type) {
            case CONF_MSG_TYPE_PHY_LINK_CONFIG:
                handle_link_config_command(spctx, msg.text);
                break;
            default:
                TLOGE("PHY Config Unknown msgtype %ld\n", msg.type);
                break;
        }
    }
}

void simphy_init_mq(SimPhyCtx *spctx)
{
    /* Initialize message queues between MAC and PHY */
    for (int nid = 0; nid < MAX_NODE_ID; nid++) {
        int mqkey_send_mac = MQ_KEY_PHY_TO_MAC + nid;
        spctx->nodes[nid].mqid_send_mac = msgget(mqkey_send_mac, IPC_CREAT | 0666);
        mq_flush(spctx->nodes[nid].mqid_send_mac);

        int mqkey_recv_mac = MQ_KEY_MAC_TO_PHY + nid;
        spctx->nodes[nid].mqid_recv_mac = msgget(mqkey_recv_mac, IPC_CREAT | 0666);
        mq_flush(spctx->nodes[nid].mqid_recv_mac);

        if (DEBUG_MQ)
            TLOGI("Message Queue ID(Node %d) : MAC->PHY %d(%d)   PHY->MAC %d(%d)\n", nid,
                  spctx->nodes[nid].mqid_recv_mac, mqkey_recv_mac,
                  spctx->nodes[nid].mqid_send_mac, mqkey_send_mac);
    }

    /* Initialize message queues between PHY and Simulator */
    int mqkey_send_report = MQ_KEY_PHY_REPORT;
    spctx->mqid_send_report = msgget(mqkey_send_report, IPC_CREAT | 0666);
    mq_flush(spctx->mqid_send_report);

    int mqkey_recv_command = MQ_KEY_PHY_COMMAND;
    spctx->mqid_recv_command = msgget(mqkey_recv_command, IPC_CREAT | 0666);
    mq_flush(spctx->mqid_recv_command);
}

/**
 * @brief Initialize link state(all connected)
 *
 * @param spctx Simphy context
 */
static void init_link_state(SimPhyCtx *spctx)
{
    for (int i = 0; i < MAX_NODE_ID; i++) {
        for (int j = 0; j < MAX_NODE_ID; j++) {
            spctx->links[i][j].los = 1;
            spctx->links[i][j].pathloss_x100 = 0;
        }
    }
}

/**
 * @brief Receive & Handle messages from MAC and Simulator
 * 
 * @param spctx Simphy context
 */
static void simphy_work(SimPhyCtx *spctx)
{
    recv_from_mac(spctx);
    recv_command(spctx);
}

SimPhyCtx *create_simphy_context()
{
    SimPhyCtx *spctx = malloc(sizeof(SimPhyCtx));
    memset(spctx, 0x00, sizeof(SimPhyCtx));
    init_link_state(spctx);
    return spctx;
}

void delete_symphy_context(SimPhyCtx *spctx)
{
    free(spctx);
}

void simphy_mainloop(SimPhyCtx *spctx)
{
    /* This function executes simphy_work() every 1ms */
    struct timespec before, after, diff, req, rem;

    while (1) {
        clock_gettime(CLOCK_REALTIME, &before);

        simphy_work(spctx); /* Do actual work */

        clock_gettime(CLOCK_REALTIME, &after);
        timespec_sub(&after, &before, &diff);
        if (diff.tv_nsec < 1000 * 1000) {
            req.tv_sec = 0;
            req.tv_nsec = 1000 * 1000 - diff.tv_nsec;
            nanosleep(&req, &rem);
        }
    }
}
