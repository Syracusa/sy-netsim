#ifndef OLSR_H
#define OLSR_H

#include <string.h>
#include <stddef.h>
#include <stdint.h>

#include "../../include/log.h"
#include "../../util/util.h"
#include "olsr_proto.h"
#include "olsr_context.h"
#include "olsr_hello.h"
#include "olsr_route.h"
#include "olsr_mantissa.h"
#include "olsr_neighbor.h"
#include "olsr_link.h"

#define DUMP_ROUTE_PKT 0

#define DUMP_HELLO_MSG 0
#define LOG_HELLO_MSG  0

#define LOG_LINK_TIMER 0


static const char *neighbor_status_str(uint8_t neighbor_code)
{
    switch (neighbor_code)
    {
    case NOT_NEIGH: return "NOT";
    case SYM_NEIGH: return "SYM";
    case MPR_NEIGH: return "MPR";
    default:
        return "N/A";
        break;
    }
}

static const char *link_status_str(uint8_t link_code)
{
    switch (link_code)
    {
    case LOST_LINK: return "LOST";
    case SYM_LINK: return "SYM ";
    case ASYM_LINK: return "ASYM";
    default:
        return "N/A ";
        break;
    }
}

#endif