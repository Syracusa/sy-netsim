#include "olsr_forwarding.h"
#include "olsr_context.h"

static int is_mpr_selector(OlsrContext *ctx,
                           in_addr_t addr)
{
    MprSelectorElem *selem = (MprSelectorElem *)
        rbtree_search(ctx->selector_tree, &addr);
    if (selem)
        return 1;
    return 0;
}

void olsr_msg_forwarding(OlsrContext *ctx,
                         OlsrMsgHeader *msg,
                         in_addr_t src)
{
    DuplicateSetKey dkey;
    dkey.orig = msg->originator;
    dkey.seq = ntohs(msg->seqno);

    DuplicateSetElem *delem =
        (DuplicateSetElem *)rbtree_search(ctx->dup_tree, &dkey);

    if (delem) {
        if (delem->retransmitted != 0 ||
            !is_mpr_selector(ctx, src)) {

            return;
        }
    } else {
        delem = malloc(sizeof(DuplicateSetElem));
        delem->rbn.key = &delem->key;

    }
}