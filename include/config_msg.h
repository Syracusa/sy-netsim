#ifndef CONFIG_MSG
#define CONFIG_MSG

#include <stdint.h>

#define CONF_MSG_TYPE_NET_SET_DUMMY_TRAFFIC         1
#define CONF_MSG_TYPE_NET_UNSET_DUMMY_TRAFFIC       2
#define CONF_MSG_TYPE_NET_UPDATE_DUMMY_TRAFFIC      3

typedef struct NetSetDummyTrafficConfig {
    uint16_t conf_id;
    uint16_t src_id;
    uint16_t dst_id;
    uint16_t payload_size;
    uint16_t interval_ms;
}__attribute__((packed)) NetSetDummyTrafficConfig;

typedef struct NetUnsetDummyTrafficConfig {
    uint16_t conf_id;
}__attribute__((packed)) NetUnsetDummyTrafficConfig;

#define CONF_MSG_TYPE_PHY_LINK_CONFIG       1
typedef struct PhyLinkConfig {
    uint8_t node_id_1;
    uint8_t node_id_2;
    uint8_t los;
    uint32_t pathloss_x100;
}__attribute__((packed)) PhyLinkConfig;

#endif