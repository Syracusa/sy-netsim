/**
 * @file sim_hdlreport.h
 * @brief Handle report messages(from simnet/mac/phy)
 */

#ifndef SIM_HDLREPORT_H
#define SIM_HDLREPORT_H

#include "simulator.h"

/** Handle report msg from simnet/mac/phy */
void recv_report(SimulatorCtx *sctx);

#endif