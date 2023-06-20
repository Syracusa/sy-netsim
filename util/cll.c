#include <stdio.h>
#include <stdlib.h>
#include "cll.h"

/* Circular Linked List Implementation */

int cll_init_head(CircularLL *head)
{
    head->next = head;
    head->prev = head;
    return 0;
}

/* Add to circular linked list */
int cll_add_tail(CircularLL *head, CircularLL *elem)
{
    if (!head || !elem)
        return -1;

    CircularLL *pprev = head->prev;

    pprev->next = elem;
    elem->prev = pprev;

    head->prev = elem;
    elem->next = head;

    return 1;
}

int cll_delete(CircularLL *head, CircularLL *delete_elem)
{
    if (!head || !delete_elem)
        return -1;

    if (head == head->next)
        return -2;

    CircularLL *trav_elem = head;

    cll_foreach(trav_elem, head)
    {
        if (trav_elem == delete_elem) {
            CircularLL *dprev = delete_elem->prev;
            CircularLL *dnext = delete_elem->next;

            dprev->next = dnext;
            dnext->prev = dprev;
            return 1; /* Success */
        }
    }

    return 0; /* Not founded */
}