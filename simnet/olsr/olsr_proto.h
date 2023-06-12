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

#define DEF_HELLO_INTERVAL_MS 2000
#define DEF_TC_INTERVAL_MS    5000

typedef struct HelloInfo {
  uint8_t link_code;
  uint8_t reserved;
  uint16_t size;
  in_addr_t neigh_addr[];
} __attribute__ ((packed)) HelloInfo;

typedef struct HelloMsg {
  uint16_t reserved;
  uint8_t htime;
  uint8_t willingness;
  HelloInfo hello_info[];
} __attribute__ ((packed)) HelloMsg;

typedef struct TcMsg {
  uint16_t ansn;
  uint16_t reserved;
  in_addr_t neigh[];
} __attribute__ ((packed)) TcMsg;

typedef struct OlsrMsgHeader {
  uint8_t olsr_msgtype;
  uint8_t olsr_vtime;
  uint16_t olsr_msgsize;
  uint32_t originator;
  uint8_t ttl;
  uint8_t hopcnt;
  uint16_t seqno;
  
  uint8_t msg_payload[];
} __attribute__ ((packed)) OlsrMsgHeader;

#endif