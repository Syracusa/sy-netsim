#ifndef CONFIG_MSG
#define CONFIG_MSG

#include <stdint.h>

typedef struct NetDummyTrafficConfig{
    uint16_t src_id;
    uint16_t dst_id;
    uint16_t payload_size;
    uint16_t interval_ms;
}__attribute__((packed)) NetDummyTrafficConfig;

#endif