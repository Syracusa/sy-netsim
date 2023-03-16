#ifndef NETUTIL_H
#define NETUTIL_H

#include <linux/ip.h>
#include <linux/udp.h>
#include <netinet/in.h>
#include <linux/if_vlan.h>
#include <linux/if_ether.h>

#define MAX_PKT_PAYLOAD 1500
#define MAX_ETHER_HEADER_MARGIN 50

typedef struct
{
    /* Packet itself*/
    unsigned char ether_margin[MAX_ETHER_HEADER_MARGIN];
    struct iphdr iph;
    struct udphdr udph;
    unsigned char payload[MAX_PKT_PAYLOAD];

    /* Additional information */
    ssize_t payload_len;
}__attribute__((packed)) PktBuf;

void build_udp_hdr_no_checksum(struct udphdr *hdr_buf,
                               uint16_t src_port,
                               uint16_t dst_port,
                               uint16_t payload_len);

void build_ip_hdr(struct iphdr *hdr_buf,
                  int len, int ttl,
                  in_addr_t srcaddr,
                  in_addr_t dstaddr,
                  unsigned char ipproto /* IPPROTO_UDP, IPPROTO_ICMP */
);


#endif