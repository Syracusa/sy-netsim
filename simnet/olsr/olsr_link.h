#ifndef OLSR_LINK_H
#define OLSR_LINK_H

#include "olsr_context.h"

void set_link_status(LinkElem *link, uint8_t status);

void link_timer_expire(void *arg);

void asym_link_timer_expire(void *arg);

void sym_link_timer_expire(void *arg);

void link_elem_expire_timer_set(OlsrContext *ctx,
                                LinkElem *link,
                                olsr_reltime vtime);

void link_elem_asym_timer_set(OlsrContext *ctx,
                              LinkElem *link,
                              olsr_reltime vtime);

void link_elem_sym_timer_set(OlsrContext *ctx,
                             LinkElem *link,
                             olsr_reltime vtime);

LinkElem *make_link_elem(OlsrContext *ctx,
                         in_addr_t src,
                         in_addr_t my_addr,
                         olsr_reltime vtime);
#endif