#include "olsr_context.h"

#include <string.h>
#include "olsr.h"

OlsrContext g_olsr_ctx;

void init_olsr_param()
{
    OlsrParam* param = &g_olsr_ctx.param;

    param->hello_interval_ms = DEF_HELLO_INTERVAL_MS;
    param->tc_interval_ms = DEF_TC_INTERVAL_MS;
    param->willingness = WILL_DEFAULT;

}

void init_olsr_context(CommonRouteConfig *config)
{
    memset(&g_olsr_ctx, 0x00, sizeof(g_olsr_ctx));

    memcpy(&g_olsr_ctx.conf, config, sizeof(CommonRouteConfig));
    g_olsr_ctx.timerqueue = create_timerqueue();
    g_olsr_ctx.olsr_tx_msgbuf = RingBuffer_new(OLSR_TX_MSGBUF_SIZE);
}