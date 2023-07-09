#ifndef PARAMS_H
#define PARAMS_H

#include <stdint.h>

#define MAX_NODE_ID 128

#define MQ_KEY_MAC_TO_NET   0x0100
#define MQ_KEY_NET_TO_MAC   0x0200
#define MQ_KEY_MAC_TO_PHY   0x0300
#define MQ_KEY_PHY_TO_MAC   0x0400

#define MQ_KEY_NET_COMMAND      0x0500
#define MQ_KEY_MAC_COMMAND      0x0600
#define MQ_KEY_PHY_COMMAND      0x0700

#define MQ_KEY_NET_REPORT       0x0800
#define MQ_KEY_MAC_REPORT       0x0900
#define MQ_KEY_PHY_REPORT       0x0a00

#define MESSAGE_TYPE_DATA       1
#define MESSAGE_TYPE_CONFIG     2
#define MESSAGE_TYPE_HEARTBEAT  3

/* Debug */
#define PKT_HEXDUMP 0
#define DEBUG_MQ 0
#define DEBUG_NET_TRX 0
#define DEBUG_MAC_TRX 0
#define DEBUG_PHY_TRX 0
#define DEBUG_PHY_DROP 0
#define DEBUG_LINK 0
#define HANDLE_SIGCHLD 0

#endif