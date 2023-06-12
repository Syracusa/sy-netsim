#include <stdio.h>
#include <stdlib.h>
#include "cll.h"

/*=== Circular Linked List Implementation ===*/

int cll_init_head(CircularLL *head)
{
    head->next = head;
    head->prev = head;
    return 0;
}

/* Add to circular linked list */
int cll_add_tail(CircularLL *head, CircularLL *elem)
{
    int res;

    CircularLL *pprev = head->prev;

    pprev->next = elem;
    elem->prev = pprev;

    head->prev = elem;
    elem->next = head;

    res = 1;

    return res;
}

int cll_delete(CircularLL *head, CircularLL *delete_elem)
{
    int res = 0;
    if (head == NULL || delete_elem == NULL)
    {
        fprintf(stderr, "[CLL] wrong pointer\n");
        res = -1;
    }
    else
    {
        CircularLL *trav_elem = head;
        if (trav_elem == trav_elem->next)
        {
            fprintf(stderr, "[CLL] No element at list\n");
        }
        else
        {
            cll_foreach(trav_elem, head)
            {
                if (trav_elem == delete_elem)
                {
                    CircularLL *dprev = delete_elem->prev;
                    CircularLL *dnext = delete_elem->next;

                    dprev->next = dnext;
                    dnext->prev = dprev;
                    res = 1;
                }
            }
        }
    }
    return res;
}