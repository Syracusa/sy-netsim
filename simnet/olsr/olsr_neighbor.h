#ifndef OLSR_NEIGHBOR_H
#define OLSR_NEIGHBOR_H

#include "olsr_context.h"

uint8_t get_neighbor_status(OlsrContext *ctx,
                            in_addr_t addr);

void set_neighbor_status(NeighborElem *neighbor, uint8_t status);

void neigh2_elem_expire(void *arg);

NeighborElem *make_neighbor_elem(OlsrContext *ctx,
                                 in_addr_t neigh_addr,
                                 uint8_t willingness);
void update_neigh2_tuple(OlsrContext *ctx,
                         NeighborElem *neigh,
                         in_addr_t n2addr,
                         olsr_reltime vtime);

void update_neighbor_status(OlsrContext *ctx,
                            NeighborElem *neigh);

#endif