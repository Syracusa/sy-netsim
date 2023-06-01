#ifndef MAC_CONNECTION_H
#define MAC_CONNECTION_H

#include "simnet.h"

void sendto_mac(SimNetCtx *snctx, void *data, size_t len, long type);

void process_mac_msg(SimNetCtx *snctx, void *data, int len);

void recvfrom_mac(SimNetCtx *snctx);

#endif