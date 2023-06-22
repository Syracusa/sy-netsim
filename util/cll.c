#include <stdio.h>
#include <stdlib.h>
#include "cll.h"

/* Circular Linked List Implementation */

int cll_init_head(CllElem *head)
{
    head->next = head;
    head->prev = head;
    return 0;
}

/* Add to circular linked list */
int cll_add_tail(CllElem *head, CllElem *elem)
{
    if (!head || !elem)
        return -1;

    CllElem *pprev = head->prev;

    pprev->next = elem;
    elem->prev = pprev;

    head->prev = elem;
    elem->next = head;

    return 1;
}

CllElem *cll_pop_head(CllElem *head)
{
    if (!head)
        return NULL;

    if (head->next != head) {
        CllElem *res = head->next;
        cll_delete(head, res);
        return res;
    }

    return NULL;
}

int cll_delete(CllElem *head, CllElem *delete_elem)
{
    if (!head || !delete_elem)
        return -1;

    CllElem *dprev = delete_elem->prev;
    CllElem *dnext = delete_elem->next;

    dprev->next = dnext;
    dnext->prev = dprev;

    return 1;
}