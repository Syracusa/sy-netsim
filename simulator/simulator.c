#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

#include "simulator.h"
#include "simulator_config.h"


static SimulatorCtx* create_simulator_context(){
    SimulatorCtx* sctx = malloc(sizeof(SimulatorCtx));
    memset(sctx, 0x00, sizeof(SimulatorCtx));    
    return sctx;
}

static void delete_simulator_context(SimulatorCtx* sctx){
    free(sctx);
}

int main() {
    TLOGI ("Start simulator...\n");
    SimulatorCtx* sctx = create_simulator_context();

    parse_config(sctx);

    delete_simulator_context(sctx);
    return 0;
}
