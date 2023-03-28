#ifndef PARAMS_H
#define PARAMS_H

#include <stdint.h>

#define MAX_NODE_ID 128

#define MQ_KEY_MAC_TO_NET   0x0100
#define MQ_KEY_NET_TO_MAC   0x0200
#define MQ_KEY_MAC_TO_PHY   0x0300
#define MQ_KEY_PHY_TO_MAC   0x0400

#define MQ_KEY_NET_DBG      0x0500
#define MQ_KEY_MAC_DBG      0x0600
#define MQ_KEY_PHY_DBG      0x0700

#define MQ_TYPE_DATA    1
#define MQ_TYPE_CONFIG  2

/* Debug */
#define PKT_HEXDUMP 0
#define DEBUG_MQ 0
#define DEBUG_NET_TRX 1
#define DEBUG_MAC_TRX 0
#define DEBUG_PHY_TRX 0

#endif