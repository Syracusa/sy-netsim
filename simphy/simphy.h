#ifndef SIMPHY_H
#define SIMPHY_H

#include <stdint.h>
#include "params.h"

/** One simulated link information */
typedef struct SimLink{
    int los;
    uint32_t pathloss_x100;
} SimLink;

/** Node state */
typedef struct 
{
    int alive; /** 1 if recvd any message */
    int mqid_send_mac; /** Message queue id PHY => MAC */
    int mqid_recv_mac; /** Message queue id MAC => PHY */
} NodeCtx;

/** Simphy context */
typedef struct
{
    NodeCtx nodes[MAX_NODE_ID]; /** State of each node */
    int mqid_recv_command; /** Message queue id Simulator => PHY */
    int mqid_send_report; /** Message queue id PHY => Simulator */
    SimLink links[MAX_NODE_ID][MAX_NODE_ID]; /** Information of every link  */
} SimPhyCtx;

/**
 * @brief Mainloop - Receive messages from MAC and Simulator
 * This function will block until termination.
 * 
 * @param spctx Simphy context
 */
void simphy_mainloop(SimPhyCtx *spctx);


/**
 * @brief Initialize message queues(MAC, Simulator)
 * 
 * @param spctx Simphy context
 */
void simphy_init_mq(SimPhyCtx *spctx);

/**
 * @brief Allocate and initialize simphy context.
 * 
 * All links are initialized as connected.
 * 
 * @return SimPhyCtx* Simphy context
 */
SimPhyCtx *create_simphy_context();

/**
 * @brief Free simphy context
 * 
 * @param spctx Simphy context
 */
void delete_symphy_context(SimPhyCtx *spctx);

#endif