#ifndef REPORT_MSG
#define REPORT_MSG

#include <stdint.h>

#include <netinet/in.h>

#define REPORT_MSG_NET_TRX                  1
#define REPORT_MSG_NET_LOCAL_TRX            2
#define REPORT_MSG_NET_ROUTING              3

typedef struct NetLocalTrxReport {
    uint32_t tx;
    uint32_t rx;
} __attribute__((packed)) NetLocalTrxReport;

typedef struct NetTrxReport {
    in_addr_t peer_addr;
    uint32_t tx;
    uint32_t rx;
}__attribute__((packed)) NetTrxReport;

typedef struct NetRoutingReport {
    uint32_t visit_num;
    in_addr_t path[];
}__attribute__((packed)) NetRoutingReport;

#endif