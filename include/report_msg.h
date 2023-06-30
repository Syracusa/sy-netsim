#ifndef REPORT_MSG
#define REPORT_MSG

#include <stdint.h>

#define REPORT_MSG_NET_TRX           1

typedef struct NetTrxReport {
    uint32_t tx;
    uint32_t rx;

    uint32_t local_tx;
    uint32_t local_rx;
}__attribute__((packed)) NetTrxReport;

#endif