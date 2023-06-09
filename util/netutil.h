#ifndef NETUTIL_H
#define NETUTIL_H

#include <stdint.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/if_vlan.h>
#include <linux/if_ether.h>

#define MAX_IPPKT_SIZE 2000
#define MAX_PKT_PAYLOAD 1500
#define MAX_ETHER_HEADER_MARGIN 50

typedef struct PktBuf
{
    /* Packet itself */
    unsigned char ether_margin[MAX_ETHER_HEADER_MARGIN];
    struct iphdr iph;
    uint8_t ip_margin[50];
    struct udphdr udph;
    unsigned char payload[MAX_PKT_PAYLOAD];

    /* Additional information */
    ssize_t iph_len;
    ssize_t payload_len;
}__attribute__((packed)) PktBuf;

#define IPUDP_HDRLEN (sizeof(struct iphdr) + sizeof(struct udphdr))

#define MAKE_BE32_IP(ipb, o1, o2, o3, o4) in_addr_t ipb; do {\
    uint8_t* _pt = (uint8_t *)(&ipb);\
    _pt[0] = (o1); _pt[1] = (o2); _pt[2] = (o3); _pt[3] = (o4);\
} while (0); 

char *ip2str(in_addr_t addr);

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

void ippkt_pack(PktBuf* pktbuf, void* buf, size_t* len);
void ippkt_unpack(PktBuf* pktbuf, void* buf, size_t len);

#endif