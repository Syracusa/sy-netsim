#ifndef CONFIG_MSG
#define CONFIG_MSG

#include <stdint.h>

#define CONF_MSG_TYPE_NET_DUMMY_TRAFFIC     1

typedef struct NetDummyTrafficConfig{
    int conf_id; // CONF_MSG_TYPE_NET_DUMMY_TRAFFIC
    uint16_t src_id;
    uint16_t dst_id;
    uint16_t payload_size;
    uint16_t interval_ms;
}__attribute__((packed)) NetDummyTrafficConfig;

#endif