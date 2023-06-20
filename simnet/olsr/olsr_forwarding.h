#ifndef OLSR_FORWARDING_H
#define OLSR_FORWARDING_H

#include "olsr_context.h"

void olsr_msg_forwarding(OlsrContext *ctx,
                         OlsrMsgHeader *msg,
                         in_addr_t src);


#endif