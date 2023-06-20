#ifndef CLL_H
#define CLL_H

#include <time.h>

/* Circular Linked List Implementation */

typedef struct _CircularLL {
    struct _CircularLL *prev;
    struct _CircularLL *next;
} CircularLL;

#define cll_foreach(trav_elem, head) \
    for ((trav_elem) = (head)->next; \
         (trav_elem) != (head);      \
         (trav_elem) = (trav_elem)->next)

#define cll_no_entry(head) \
    ((head)->next == head)

#define cll_one_entry(head) \
    ((head)->next->next == head)

int cll_add_tail(CircularLL *head, CircularLL *elem);
int cll_delete(CircularLL *head, CircularLL *delete_elem);
int cll_init_head(CircularLL *head);

#endif