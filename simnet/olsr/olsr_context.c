#include "olsr_context.h"

#include <string.h>

OlsrContext g_olsr_ctx;

void init_olsr_context()
{
    memset(&g_olsr_ctx, 0x00, sizeof(g_olsr_ctx));

    g_olsr_ctx.olsr_tx_msgbuf = RingBuffer_new(OLSR_TX_MSGBUF_SIZE);
}