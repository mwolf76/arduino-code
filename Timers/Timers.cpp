/**
 * @file Timers.cpp
 * @brief Timers library implementation
 *
 * Copyright (C) 2013 Marco Pensallorto
 * < marco DOT pensallorto AT gmail DOT com >
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
**/
#include <Timers.h>
#include <Debug.h>

#include <stdio.h>
#include <string.h>
#include <limits.h>

/* -- static data ----------------------------------------------------------- */

/* configurable parameters (see timers_init) */
static int _tmrs_max_timeouts;

static timer_t _tmrs_array[MAX_TIMERS];
static timer_t *_tmrs_free_list;
static timer_t *_tmrs_active_list;

static int _tmrs_initialized = 0;
static timer_id_t _tmrs_next_id = 0;

/** -- static function prototypes ------------------------------------------- */
static inline int timers_cmp( timer_t *a, timer_t *b );
static inline void timers_set( timer_t *timer, ticks_t base, ticks_t dly);

static int timers_array_insert( timer_t *timer );
static int timers_array_remove( timer_t *timer );

/** -- public functions ----------------------------------------------------- */
int timers_is_initialized()
{ return _tmrs_initialized; }

int timers_init(int max_simultaneous_timeouts)
{
    int i = MAX_TIMERS - 1;

    _tmrs_free_list = NULL;
    _tmrs_active_list = NULL;

    while (0 <= i) {
        _tmrs_array[i].next = _tmrs_free_list;
        _tmrs_free_list = &_tmrs_array[i];
        -- i;
    }

    _tmrs_max_timeouts = max_simultaneous_timeouts;
    _tmrs_initialized = 1;

    return 0;
}

/* returns timer id, -1 on failure. */
timer_id_t timers_schedule( ticks_t dly, timer_handler_t handler,
                            void *user_data)
{
    timer_t timer;
    ASSERT(timers_is_initialized());

    /* populate data structure */
    timer.id =  _tmrs_next_id ++;
    timers_set( &timer, millis(), dly);
    timer.handler = handler;
    timer.user_data = user_data;

    return (0 == timers_array_insert( &timer ))
        ? timer.id : -1;
}

/* returns number of milliseconds before expiration, 0 if already
   expired, UINT_MAX if not found */
ticks_t timers_timeleft(timer_id_t id)
{
    timer_t *head = _tmrs_active_list;
    ASSERT(timers_is_initialized());

    while (NULL != head) {

        if (head->id == id) {
            ticks_t deadline = head->base + head->dly;
            ticks_t now = millis();
            return deadline > now
                ? deadline - now
                : 0;
        }

        head = head->next;
    } /* while */

    return UINT_MAX; /* not found */
}

int timers_cancel(timer_id_t id)
{
    timer_t *head = _tmrs_active_list;
    ASSERT(timers_is_initialized());

    while (NULL != head) {

        if (head->id == id) {
            int rc = timers_array_remove(head);
            ASSERT (0 == rc);

            return 0;
        }

        head = head->next;
    } /* while */

    return -1; /* not found */
}

void timers_check()
{
    int rc, count = _tmrs_max_timeouts;
    timer_t *next, *head;
    ASSERT(timers_is_initialized());

    static ticks_t last = NO_TICKS;
    ticks_t now = millis();

    /* clock overflow detected, clear is_future flags */
    if (now < last) {
        head = _tmrs_active_list;
        while (NULL != head) {
            ASSERT(0 <= head->is_future);
            -- head->is_future;
            head = head->next;
        }
    }
    last = now; /* save current clock ticks */

    head = _tmrs_active_list;
    while (NULL != head) {

        if (1 == head->is_future ||
            (0 == head->is_future && now < head->base + head->dly))
            /* -1 == head->is_future means the timer *has* expired */
            break;

        next = head->next;

        if (! head->handler(head->id, now, head->user_data)) {
            rc = timers_array_remove(head);
            ASSERT(0 == rc);
        }
        else {
            /* reschedule */
            timers_set(head, now, head->dly);
        }

        if (0 == -- count)
            break;

        head = next;
    } /* while */
}


/* -- static functions ------------------------------------------------------ */
static inline int timers_cmp( timer_t *a, timer_t *b )
{
    /* b is in the future, a is not => a comes first */
    if (b->is_future > a->is_future)
        return 1;

    /* a is in the future, b is not => b comes first */
    if (a->is_future > b->is_future)
        return -1;

    /* here a and b are either both present or both future, hence
       ticks have the same interpretation */
    ticks_t ta = a->base + a->dly;
    ticks_t tb = b->base + b->dly;
    return (ta <= tb) ? 1 : -1;
}

static int timers_array_insert( timer_t *timer )
{
    if (_tmrs_free_list == NULL)
        return -1;

    /* fetch head from free list */
    timer_t *elem = _tmrs_free_list;
    _tmrs_free_list = elem->next;

    /* copy timer data  (avoid overlap) */
    if (elem != timer) {
        memcpy( elem, timer, sizeof(timer_t));
    }

    /* sorted insertion */
    timer_t *previous = NULL, *eye = _tmrs_active_list;
    while (NULL != eye && 0 < timers_cmp(eye, elem)) {
        previous = eye;
        eye = eye->next;
    }

    if (NULL == previous) {
        _tmrs_active_list = elem;
    }
    else {
        previous->next = elem;
    }

    elem->next = eye;
    return 0;
}

static int timers_array_remove(timer_t *timer)
{
    timer_t *previous = NULL, *head = _tmrs_active_list;

    if (NULL == head)
        return -1;

    while (NULL != head && head != timer) {
        previous = head;
        head = head->next;
    }

    if (head != timer)
        return -1;

    if (NULL == previous) {
        _tmrs_active_list = head->next;
    }
    else {
        previous->next = head->next;
    }

    /* put block back into free list */
    timer->next = _tmrs_free_list;
    _tmrs_free_list = timer;

    return 0;
}

static inline void timers_set(timer_t *timer, ticks_t base, ticks_t dly)
{
    timer->base = base;
    timer->dly  = dly;

    /* is_future signals arithmetic overflow has occurred, timer is postponed */
    timer->is_future = (base + dly < base) ? 1 : 0;
}
