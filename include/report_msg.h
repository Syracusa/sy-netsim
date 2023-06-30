#ifndef REPORT_MSG
#define REPORT_MSG

#include <stdint.h>

#include <netinet/in.h>

#define REPORT_MSG_NET_TRX                  1
#define REPORT_MSG_NET_LOCAL_TRX            2

typedef struct NetLocalTrxReport {
    uint32_t tx;
    uint32_t rx;
} __attribute__((packed)) NetLocalTrxReport;

typedef struct NetTrxReport {
    in_addr_t peer_addr;
    uint32_t tx;
    uint32_t rx;
}__attribute__((packed)) NetTrxReport;

#endif