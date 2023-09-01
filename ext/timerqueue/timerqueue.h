/**
 * @file timerqueue.h
 * @brief Timerqueue for single thread event loop
 * User should call timerqueue_work() periodically to make this work.
 * 
 * @details
 * This timerqueue is based on red-black tree(taken from freebsd sourcecode).
 * When user call timerqueue_work(), this will check the first element of 
 * rbtree. The rbtree's key is the expire time of the element so the first
 * element is the element that will be expired first.
 * If the first element's expire time is passed, this will call the callback
 * function and check the next element.
 */

#ifndef TIMERQUEUE_H
#define TIMERQUEUE_H

#include <time.h>
#include "rbtree.h"

 /** Rbtree that contains TimerqueueElem */
typedef struct {
    rbtree_type *rbt; /** Rbtree elem = TimerqueueElem */
} TimerqueueContext;

/** Rbtree(TimerqueueContext) key - time + elem pointer(to avoid collision) */
typedef struct {
    struct timespec expire; /** The time that callback should executed */
    void *ptr; /** Prevent key collision */
} TqKey;

typedef struct {
    rbnode_type priv_rbn; /** Private - Rbnode */
    TqKey priv_rbk; /** Private - Rbnode key */

    /** 
     * Microsecond scale jitter config 
     * User can set this value to make jitter.
    */
    int max_jitter;

    /** 
     * if 0 then timerqueue will detach this node
     * 
     * User can read this value to check if this element is active 
     * User can modify this value to 0. That will cause detach on expire.
     * Modify this value to 1 from user code will cause undefined behavior. 
     */
    int active;

    /**
     * If 0 then one can safely free this elem
     * If not, call free() to this elem will cause segfault
     * 
     * User can read this value to check if this element can be safely freed.
     * User SHOULD NOT modify this value.
     */
    int attached;

    /**
     * If 1, this elem will be automatically freed on detach
     * If user call another free() to this elem, that will cause a double free
     * error
     * 
     * User can modify this value(0 or 1).
     */
    int free_on_detach;

    /** For debug purpose. This will be printed when callback is null */
    char debug_name[50];

    /** 
     * Callback execution interval 
     * 
     * If user change this value, The change will be applied after current 
     * expire time.
     * If user want to change this value immediately, user should call
     * timerqueue_reactivate_timer()
     */
    int interval_us;

    /**
     * If 1, this timer will be called only once.
     * (will be automatically deactived and detached)
     * 
     * User can modify this value(0 or 1).
     */
    int use_once;

    /** This pointer will be provided as param to callback function */
    void *arg;

    /** Actual callback function */
    void (*callback)(void *arg);

    /** This will be called when detached(if not null) */
    void (*detached_callback)(void *arg);
} TimerqueueElem;


/**
 * @brief Allocate new timerqueue context and initialize it.
 * 
 * @return TimerqueueContext* 
 */
TimerqueueContext *create_timerqueue();

/**
 * @brief Create new timerqueue element
 * 
 * @details This will allocate new TimerqueueElem and initialize key pointer.
 * @return TimerqueueElem* A pointer to new TimerqueueElem
 */
TimerqueueElem *timerqueue_new_timer();

/**
 * @brief Register TimerqueueElem to timerqueue.
 * 
 * @param tq Timerqueue that element will be registered
 * @param elem Element that will be registered
 */
void timerqueue_register_timer(TimerqueueContext *tq, TimerqueueElem *elem);

/**
 * @brief Check time and call callback function if needed.
 * 
 * @param tq Timerqueue that will be checked
 */
void timerqueue_work(TimerqueueContext *tq);

/**
 * @brief Reregister timer to timerqueue regardless of it's state.
 * 
 * @param tq Timerqueue that element is registered
 * @param elem Element that will be registered
 */
void timerqueue_reactivate_timer(TimerqueueContext *tq, TimerqueueElem *elem);

/**
 * @brief Free timerqueue safely.
 * 
 * @param tq Timerqueue that will be freed
 */
void timerqueue_free_timer(TimerqueueElem *elem);


#endif