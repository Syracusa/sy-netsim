#include <stdio.h>
#include <time.h>
#include <stdlib.h> 
#include <string.h>

#include "rbtree.h"
#include "timerqueue.h"

#define timespec_sub(after, before, result)                         \
  do {                                                              \
    (result)->tv_sec = (after)->tv_sec - (before)->tv_sec;          \
    (result)->tv_nsec = (after)->tv_nsec - (before)->tv_nsec;       \
    if ((result)->tv_nsec < 0) {                                    \
      --(result)->tv_sec;                                           \
      (result)->tv_nsec += 1000000000;                              \
    }                                                               \
  } while (0)


static inline int timespec_add_usec(struct timespec *t,
                                    unsigned long usec)
{
    unsigned long long add;
    int res = 0;

    if (!t) {
        res = -1;
    } else {
        add = t->tv_nsec + (usec * 1000);
        t->tv_sec += add / 1000000000;
        t->tv_nsec = add % 1000000000;
    }

    return res;
}

static inline int check_expire(struct timespec *expiretime,
                               struct timespec *currtime)
{
    int res = 0;
    if (expiretime->tv_sec == currtime->tv_sec)
        res = expiretime->tv_nsec <= currtime->tv_nsec ? 1 : 0;
    else
        res = expiretime->tv_sec < currtime->tv_sec ? 1 : 0;
    return res;
}

static int compare_elem(const void *k1, const void *k2)
{
    TqKey *n1 = (TqKey *)k1;
    TqKey *n2 = (TqKey *)k2;

    if (n1->expire.tv_nsec == n2->expire.tv_nsec &&
        n1->expire.tv_sec == n2->expire.tv_sec) {
        if (n1->ptr > n2->ptr) {
            return 1;
        } else if (n1->ptr < n2->ptr) {
            return -1;
        } else {
            return 0;
        }
    } else if (check_expire(&n1->expire, &n2->expire)) {
        return -1;
    } else {
        return 1;
    }
}

TimerqueueElem *timerqueue_new_timer()
{
    TimerqueueElem *elem = malloc(sizeof(TimerqueueElem));
    memset(elem, 0x00, sizeof(TimerqueueElem));

    elem->priv_rbk.ptr = elem;
    elem->rbn.key = &elem->priv_rbk;

    return elem;
}

void timerqueue_free_timer(TimerqueueElem *elem)
{
    elem->active = 0;
    if (elem->attached) {
        elem->free_on_detach = 1;
    } else {
        free(elem);
    }
}

void timerqueue_register_timer(TimerqueueContext *tq, TimerqueueElem *elem)
{
    clock_gettime(CLOCK_REALTIME, &elem->priv_rbk.expire);
    timespec_add_usec(&elem->priv_rbk.expire, elem->interval_us);

    rbtree_insert(tq->rbt, (rbnode_type *)elem);
    elem->attached = 1;
    elem->active = 1;
}

void timerqueue_reactivate_timer(TimerqueueContext *tq, TimerqueueElem *elem)
{
    if (elem->attached == 1) {
        rbtree_delete(tq->rbt, elem->rbn.key);
        elem->attached = 0;
    }
    timerqueue_register_timer(tq, elem);
}

void timerqueue_work(TimerqueueContext *tq)
{
    struct timespec currtime;
    clock_gettime(CLOCK_REALTIME, &currtime);

    TimerqueueElem *first = (TimerqueueElem *)rbtree_first(tq->rbt);
    if (first == (TimerqueueElem *)RBTREE_NULL) {
        return;
    }

    while (check_expire(&(first->priv_rbk.expire), &currtime)) {
        rbtree_delete(tq->rbt, first->rbn.key);
        if (first->active) {
            if (!first->callback)
                fprintf(stderr, "NULL callback! (%s)\n", first->debug_name);
            first->callback(first->arg);
            if (first->use_once) {
                first->active = 0;
                first->attached = 0;
            } else {
                int new_interval = first->interval_us;
                if (first->max_jitter) {
                    new_interval += (rand() % 100 + 1) * (first->max_jitter * 2) / 100;
                    new_interval -= first->max_jitter;
                }
                timespec_add_usec(&first->priv_rbk.expire, new_interval);
                rbtree_insert(tq->rbt, (rbnode_type *)first);
            }
        } else {
            first->attached = 0;
        }

        if (first->attached == 0) {
            if (first->detached_callback)
                first->detached_callback(first->arg);
            if (first->free_on_detach)
                free(first);
        }

        first = (TimerqueueElem *)rbtree_first(tq->rbt);
    }
}

TimerqueueContext *create_timerqueue()
{
    TimerqueueContext *tq = malloc(sizeof(TimerqueueContext));
    tq->rbt = rbtree_create(compare_elem);
    return tq;
}