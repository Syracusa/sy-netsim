#include "olsr_tc.h"
#include "olsr.h"

void build_olsr_tc(OlsrContext* ctx, void* buf, size_t* len)
{
    uint8_t *offset = buf;
    uint8_t *start = buf;

    TcMsg *tc_msg = (TcMsg *)buf;
    offset += sizeof(TcMsg);
    
    tc_msg->ansn = htons(ctx->ansn);
    
    int neighidx = 0;
    MprSelectorElem* sel_elem;
    RBTREE_FOR(sel_elem, MprSelectorElem*, ctx->selector_tree)
    {
        tc_msg->neigh[neighidx] = sel_elem->selector_addr;
        neighidx++;
        offset += sizeof(in_addr_t);
    }
    
    *len = offset - start;
}