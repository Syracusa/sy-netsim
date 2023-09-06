#include <stdio.h>

#include "log.h"
#include "simphy.h"

int main()
{
    sprintf(dbgname, "PHY   ");
    dbgfile = stderr;
    TLOGI("SIMPHY STARTED\n");
    SimPhyCtx *spctx = create_simphy_context();

    simphy_init_mq(spctx);
    simphy_mainloop(spctx);

    delete_symphy_context(spctx);

    return 0;
}