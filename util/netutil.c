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

void build_udp_hdr_no_checksum(void *hdr_buf,
                               uint16_t src_port,
                               uint16_t dst_port,
                               uint16_t payload_len /* Only payload, Udp hdr len should not be added */)
{
    struct udphdr *hdr = hdr_buf;
    hdr->source = htons(src_port);
    hdr->dest = htons(dst_port);
    hdr->len = htons(sizeof(struct udphdr) + payload_len);
    hdr->check = 0; /* No checksum */
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

void build_ip_hdr(void *hdr_buf,
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
    hdr->id = 0;       /* use when ip is fragmented */
    hdr->frag_off = 0; /* use when ip is fragmented */
    hdr->ttl = ttl;
    hdr->protocol = ipproto;
    hdr->check = 0;
    hdr->saddr = srcaddr;
    hdr->daddr = dstaddr;
    hdr->check = calc_ip_checksum(hdr, sizeof(struct iphdr));
}

void minimal_mip_encap(PacketBuf *ip_pkt,
                       PacketBuf *encap_ip_pkt_buf,
                       in_addr_t new_src,
                       in_addr_t new_dst)
{
    struct iphdr *old_ip_hdr = (struct iphdr *)ip_pkt->data;
    struct iphdr *new_ip_hdr = (struct iphdr *)encap_ip_pkt_buf->data;

    int old_ip_hdr_len = old_ip_hdr->ihl * 4;
    memcpy(new_ip_hdr, old_ip_hdr, old_ip_hdr_len);

    MinimalMobileIpHdr *mip_hdr =
        (MinimalMobileIpHdr *)&(encap_ip_pkt_buf->data[old_ip_hdr_len]);
    mip_hdr->ipproto = old_ip_hdr->protocol;
    mip_hdr->recv = 0; /* reserved */
    mip_hdr->s = 1; /* Original source address will be preserved */
    mip_hdr->checksum = 0;
    mip_hdr->orig_dst_ip = old_ip_hdr->daddr;
    mip_hdr->orig_src_ip = old_ip_hdr->saddr;
    mip_hdr->checksum = calc_ip_checksum(mip_hdr, sizeof(MinimalMobileIpHdr));

    int new_ip_hdr_len = old_ip_hdr_len + sizeof(MinimalMobileIpHdr);
    memcpy(&(encap_ip_pkt_buf->data[new_ip_hdr_len]),
           &(ip_pkt->data[old_ip_hdr_len]),
           ip_pkt->length - old_ip_hdr_len);

    new_ip_hdr->protocol = IPPROTO_MOBILE;
    new_ip_hdr->ihl = new_ip_hdr_len / 4;
    new_ip_hdr->tot_len = htons(ip_pkt->length + sizeof(MinimalMobileIpHdr));
    new_ip_hdr->saddr = new_src;
    new_ip_hdr->daddr = new_dst;
    new_ip_hdr->check = 0;
    new_ip_hdr->check = calc_ip_checksum(new_ip_hdr, new_ip_hdr_len);

    encap_ip_pkt_buf->length = sizeof(MinimalMobileIpHdr) + ip_pkt->length;
}

void minimal_mip_decap(PacketBuf *encap_ip_pkt,
                       PacketBuf *decap_ip_pkt_buf)
{
    struct iphdr *old_ip_hdr = (struct iphdr *)encap_ip_pkt->data;

    int old_ip_hdr_len = old_ip_hdr->ihl * 4;
    int decap_ip_hdr_len = old_ip_hdr_len - sizeof(MinimalMobileIpHdr);
    MinimalMobileIpHdr *mip_hdr =
        (MinimalMobileIpHdr *)&(encap_ip_pkt->data[decap_ip_hdr_len]);

    struct iphdr *new_ip_hdr = (struct iphdr *)decap_ip_pkt_buf->data;
    memcpy(new_ip_hdr, old_ip_hdr, decap_ip_hdr_len);

    new_ip_hdr->protocol = mip_hdr->ipproto;
    new_ip_hdr->ihl = decap_ip_hdr_len / 4;
    new_ip_hdr->tot_len =
        htons(encap_ip_pkt->length - sizeof(MinimalMobileIpHdr));
    new_ip_hdr->saddr = mip_hdr->orig_src_ip;
    new_ip_hdr->daddr = mip_hdr->orig_dst_ip;
    new_ip_hdr->check = 0;
    new_ip_hdr->check = calc_ip_checksum(new_ip_hdr, decap_ip_hdr_len);

    memcpy(&(decap_ip_pkt_buf->data[decap_ip_hdr_len]),
           &(encap_ip_pkt->data[old_ip_hdr_len]),
           encap_ip_pkt->length - old_ip_hdr_len);

    decap_ip_pkt_buf->length = encap_ip_pkt->length - sizeof(MinimalMobileIpHdr);
}