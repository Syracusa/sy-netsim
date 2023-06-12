#ifndef OLSR_PROTO_H
#define OLSR_PROTO_H

#define OLSR_PROTO_PORT 698

#include <stdint.h>

struct hellinfo {
  uint8_t link_code;
  uint8_t reserved;
  uint16_t size;
  uint32_t neigh_addr[1];              /* neighbor IP address(es) */
} __attribute__ ((packed));

struct hellomsg {
  uint16_t reserved;
  uint8_t htime;
  uint8_t willingness;
  struct hellinfo hell_info[1];
} __attribute__ ((packed));

struct neigh_info {
  uint32_t addr;
} __attribute__ ((packed));

struct olsr_tcmsg {
  uint16_t ansn;
  uint16_t reserved;
  struct neigh_info neigh[1];
} __attribute__ ((packed));

struct olsrmsg {
  uint8_t olsr_msgtype;
  uint8_t olsr_vtime;
  uint16_t olsr_msgsize;
  uint32_t originator;
  uint8_t ttl;
  uint8_t hopcnt;
  uint16_t seqno;

  union {
    struct hellomsg hello;
    struct olsr_tcmsg tc;
  } message;

} __attribute__ ((packed));
#endif