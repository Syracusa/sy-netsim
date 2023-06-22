#ifndef CLL_H
#define CLL_H

#include <time.h>

/* Circular Linked List Implementation */

typedef struct _CllElem {
    struct _CllElem *prev;
    struct _CllElem *next;
} CllElem;

typedef CllElem CllHead;

#define cll_foreach(trav_elem, head) \
    for ((trav_elem) = (head)->next; \
         (trav_elem) != (head);      \
         (trav_elem) = (trav_elem)->next)

#define cll_no_entry(head) \
    ((head)->next == head)

#define cll_one_entry(head) \
    ((head)->next->next == head)

int cll_add_tail(CllElem *head, CllElem *elem);
CllElem *cll_pop_head(CllElem *head);
int cll_delete(CllElem *head, CllElem *delete_elem);
int cll_init_head(CllElem *head);

#endif