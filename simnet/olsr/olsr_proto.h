#ifndef OLSR_PROTO_H
#define OLSR_PROTO_H

#include <stdint.h>
#include <netinet/in.h>

#define OLSR_PROTO_PORT 698

#define MSG_TYPE_HELLO         1
#define MSG_TYPE_TC            2

#define WILL_NEVER            0
#define WILL_LOW              1
#define WILL_DEFAULT          3
#define WILL_HIGH             6
#define WILL_ALWAYS           7

#define NOT_NEIGH             0
#define SYM_NEIGH             1
#define MPR_NEIGH             2
#define NEIGHBOR_STATUS_END   3

#define UNSPEC_LINK           0
#define ASYM_LINK             1
#define SYM_LINK              2
#define LOST_LINK             3
#define LINK_STATUS_END       4

#define STATUS_UNAVAILABLE 255


#define TEST_FAST_ROUTE        1

#if TEST_FAST_ROUTE
#define DEF_HELLO_INTERVAL_MS   500
#define DEF_REFRESH_INTERVAL_MS 500
#define DEF_TC_INTERVAL_MS      1250
#else
#define DEF_HELLO_INTERVAL_MS   2000
#define DEF_REFRESH_INTERVAL_MS 2000
#define DEF_TC_INTERVAL_MS      5000
#endif

#define DEF_NEIGHB_HOLD_TIME_MS    3 * DEF_REFRESH_INTERVAL_MS
#define DEF_TOP_HOLD_TIME_MS       3 * DEF_TC_INTERVAL_MS
// #define DEF_DUP_HOLD_TIME_MS       30 * 1000
#define DEF_DUP_HOLD_TIME_MS       10 * 1000

#define DEF_HELLO_VTIME         DEF_NEIGHB_HOLD_TIME_MS
#define DEF_TC_VTIME            DEF_TOP_HOLD_TIME_MS

#define CREATE_LINK_CODE(status, link) (link | (status<<2))

#define EXTRACT_STATUS(link_code) ((link_code & 0xC)>>2)

#define EXTRACT_LINK(link_code) (link_code & 0x3)

typedef struct HelloInfo
{
  uint8_t link_code;
  uint8_t reserved;
  uint16_t size;
  in_addr_t neigh_addr[];
} __attribute__((packed)) HelloInfo;

typedef struct HelloMsg
{
  uint16_t reserved;
  uint8_t htime;
  uint8_t willingness;
  HelloInfo hello_info[];
} __attribute__((packed)) HelloMsg;

typedef struct TcMsg
{
  uint16_t ansn;
  uint16_t reserved;
  in_addr_t neigh[];
} __attribute__((packed)) TcMsg;

typedef struct OlsrMsgHeader
{
  uint8_t olsr_msgtype;
  uint8_t olsr_vtime;
  uint16_t olsr_msgsize;
  uint32_t originator;
  uint8_t ttl;
  uint8_t hopcnt;
  uint16_t seqno;

  uint8_t msg_payload[];
} __attribute__((packed)) OlsrMsgHeader;

#endif