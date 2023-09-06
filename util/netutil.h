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
#define MAX_ETHER_HEADER_MARGIN 50

#define IP_C_CLASS_MATCH(ip1, ip2) (((ip1) & 0xffffff00) == ((ip2) & 0xffffff00))

#define IPUDP_HDRLEN (sizeof(struct iphdr) + sizeof(struct udphdr))

/** Declare and initiate in_addr_t from 4 octets */
#define MAKE_BE32_IP(ipb, o1, o2, o3, o4) in_addr_t ipb; do {\
    uint8_t* _pt = (uint8_t *)(&ipb);\
    _pt[0] = (o1); _pt[1] = (o2); _pt[2] = (o3); _pt[3] = (o4);\
} while (0); 

/** Fixed size packet buffer */
typedef struct PacketBuf
{
    size_t length;
    unsigned char data[MAX_IPPKT_SIZE + MAX_ETHER_HEADER_MARGIN];
}__attribute__((packed)) PacketBuf;

#define IPPROTO_MOBILE 55 

typedef struct MinimalMobileIpHdr
{
    /** IPPROTO_MOBILE = 55 */
    uint8_t ipproto;

    /** Reserved */
    uint8_t recv : 7;

    /** Original Source Address Present. Always 1 in this implementation */
    uint8_t s : 1;

    /**
     * Header Checksum
     *
     * The 16-bit one's complement of the one's complement sum of all
     * 16-bit words in the minimal forwarding header.  For purposes of
     * computing the checksum, the value of the checksum field is 0.
     * The IP header and IP payload (after the minimal forwarding
     * header) are not included in this checksum computation.
     *  - RFC 2004(https://datatracker.ietf.org/doc/html/rfc2004)
     */
    uint16_t checksum;

    /** Original Destination Address */
    uint32_t orig_dst_ip;

    /** Original Source Address */
    uint32_t orig_src_ip;
}__attribute__((packed)) MinimalMobileIpHdr;

/**
 * @brief Return string representation of IP address
 * Internally, this function uses 100 static buffers to store string.
 * 
 * @param addr IP address to be converted
 * @return char* Converted string
 */
char *ip2str(in_addr_t addr);

/**
 * @brief Write UDP header to buf without checksum
 * 
 * @param payload_len Only payload, Udp hdr len should not be added 
 */
void build_udp_hdr_no_checksum(void *hdr_buf,
                               uint16_t src_port,
                               uint16_t dst_port,
                               uint16_t payload_len);


/**
 * @brief Build UDP header
 * 
 * @param hdr_buf Buffer to store IP header
 * @param len Length of IP header + UDP header + payload
 * @param ttl Time to live
 * @param srcaddr Source IP address
 * @param dstaddr Destination IP address
 * @param ipproto IP protocol number(IPPROTO_UDP, IPPROTO_ICMP)
 */
void build_ip_hdr(void *hdr_buf,
                  int len,
                  int ttl,
                  in_addr_t srcaddr,
                  in_addr_t dstaddr,
                  unsigned char ipproto
);

/**
 * @brief Minimal IP Encapsulation RFC 2004
 * 
 * @param ip_pkt IP packet to be encapsulated
 * @param encap_ip_pkt_buf Buffer to store encapsulated IP packet
 * @param new_src New source address
 * @param new_dst New destination address
 */
void minimal_mip_encap(PacketBuf *ip_pkt,
                       PacketBuf *encap_ip_pkt_buf,
                       in_addr_t new_src,
                       in_addr_t new_dst);


/**
 * @brief Minimal IP Decapsulation RFC 2004
 * 
 * @param encap_ip_pkt Encapsulated IP packet to be decapsulated
 * @param decap_ip_pkt_buf Buffer to store decapsulated IP packet
 */
void minimal_mip_decap(PacketBuf *encap_ip_pkt,
                       PacketBuf *decap_ip_pkt_buf);

#endif