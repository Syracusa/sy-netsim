#include <stdlib.h>
#include <string.h>
#include "netutil.h"

char *ip2str(in_addr_t addr)
{
    static int bufidx = 0;
    static char ipstr[100][INET_ADDRSTRLEN];
    if (++bufidx > 99)
        bufidx = 0;
    inet_ntop(AF_INET, &(addr), ipstr[bufidx], INET_ADDRSTRLEN);
    return ipstr[bufidx];
}

void build_udp_hdr_no_checksum(struct udphdr *hdr_buf,
                               uint16_t src_port,
                               uint16_t dst_port,
                               uint16_t payload_len /* Only payload, Udp hdr len should not be added */)
{
    hdr_buf->source = htons(src_port);
    hdr_buf->dest = htons(dst_port);
    hdr_buf->len = htons(sizeof(struct udphdr) + payload_len);
    hdr_buf->check = 0; /* No checksum */
}

static uint16_t calc_ip_checksum(void *vdata, size_t length)
{
    // Cast the data pointer to one that can be indexed.
    char *data = (char *)vdata;

    // Initialise the accumulator.
    uint32_t acc = 0xffff;

    // Handle complete 16-bit blocks.
    size_t i;
    for (i = 0; i + 1 < length; i += 2) {
        uint16_t word;
        memcpy(&word, data + i, 2);
        acc += ntohs(word);
        if (acc > 0xffff) {
            acc -= 0xffff;
        }
    }

    // Handle any partial block at the end of the data.
    if (length & 1) {
        uint16_t word = 0;
        memcpy(&word, data + length - 1, 1);
        acc += ntohs(word);
        if (acc > 0xffff) {
            acc -= 0xffff;
        }
    }

    // Return the checksum in network byte order.
    return htons(~acc);
}

void build_ip_hdr(struct iphdr *hdr_buf,
                  int len, /* iph + udph + payload */
                  int ttl,
                  in_addr_t srcaddr,
                  in_addr_t dstaddr,
                  unsigned char ipproto /* IPPROTO_UDP, IPPROTO_ICMP */
)
{
    struct iphdr *hdr = hdr_buf;
    hdr->ihl = 5;     /* ip hdr length scale of 32-bits */
    hdr->version = 4; /* ipv4 hdr */
    hdr->tos = 0;
    hdr->tot_len = htons(len);
    hdr->id = 0;       /* use when ip is fregmented */
    hdr->frag_off = 0; /* use when ip is fregmented */
    hdr->ttl = ttl;    /* max 4-hop */
    hdr->protocol = ipproto;
    hdr->check = 0;
    hdr->saddr = srcaddr;
    hdr->daddr = dstaddr;
    hdr->check = calc_ip_checksum(hdr, sizeof(struct iphdr));
}

void ippkt_pack(PktBuf *pktbuf, void *buf, size_t *len)
{
    uint8_t *orig = buf;
    uint8_t *ptr = buf;

    memcpy(ptr, &pktbuf->iph, pktbuf->iph_len);
    ptr += pktbuf->iph_len;

    memcpy(ptr, &pktbuf->udph, sizeof(struct udphdr));
    ptr += sizeof(struct udphdr);

    memcpy(ptr, &pktbuf->payload, pktbuf->payload_len);
    ptr += pktbuf->payload_len;

    *len = ptr - orig;
}

void ippkt_unpack(PktBuf *pktbuf, void *buf, size_t len)
{
    uint8_t *ptr = buf;

    struct iphdr* ipp;
    ipp = (struct iphdr*)buf;

    pktbuf->iph_len = ipp->ihl * 4;
    memcpy(&pktbuf->iph, ptr, pktbuf->iph_len);
    ptr += pktbuf->iph_len;

    memcpy(&pktbuf->udph, ptr, sizeof(struct udphdr));
    ptr += sizeof(struct udphdr);
    
    pktbuf->payload_len = len - (pktbuf->iph_len + sizeof(struct udphdr));
    memcpy(&pktbuf->payload, ptr, pktbuf->payload_len);
    ptr += pktbuf->payload_len;
}

/* Minimal IP Encapsulation RFC 2004 */
// void minimal_mip_encap(void* ippkt, ) {

// }

// void minimal_mip_decap() {

// }