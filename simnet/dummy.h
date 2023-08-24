#ifndef DUMMY_H
#define DUMMY_H

#include "simnet.h"

void register_dummypkt_send_job(SimNetCtx *snctx,
                                NetSetDummyTrafficConfig *conf);

void unregister_dummypkt_send_job(SimNetCtx *snctx,
                                  NetUnsetDummyTrafficConfig *conf);

#endif