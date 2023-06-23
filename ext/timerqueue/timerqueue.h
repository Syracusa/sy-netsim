#ifndef TIMERQUEUE_H
#define TIMERQUEUE_H


#include <time.h>
#include "rbtree.h"

typedef struct {
    rbtree_type *rbt;
} TimerqueueContext;

typedef struct {
    struct timespec expire;
    void *ptr;
} TqKey;

typedef struct {
    /* Privates */
    rbnode_type rbn;
    TqKey priv_rbk;
    int max_jitter;

    /* if 0 then timerqueue will detatch this node */
    int active;

    /* If 0 then safely free this elem */
    int attached;
    int free_on_detach;

    char debug_name[50];

    /* User should write these field */
    int interval_us;
    int use_once;
    void *arg;
    void (*callback)(void *arg);
    void (*detached_callback)(void *arg);
} TimerqueueElem;

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

TimerqueueElem *timerqueue_new_timer();

void timerqueue_free_timer(TimerqueueElem *timer);

void timerqueue_reset_expire_time(TimerqueueElem *elem);

void timerqueue_register_timer(TimerqueueContext *tq, TimerqueueElem *elem);

void timerqueue_reactivate_timer(TimerqueueContext *tq, TimerqueueElem *elem);

TimerqueueContext *create_timerqueue();

void timerqueue_work(TimerqueueContext *tq);

#endif