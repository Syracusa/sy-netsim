#ifndef NET_REPORT_H
#define NET_REPORT_H

#include "simnet.h"

/**
 * Start report to simulator(1s interval)
 * Only sent if data changes
*/
void start_report_job(SimNetCtx *snctx);

#endif