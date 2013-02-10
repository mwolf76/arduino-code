/**
 * @file Microtimers.cpp
 * @brief Microtimers library implementation
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
#include <Microtimers.h>
#include <Debug.h>

#include <stdio.h>
#include <string.h>
#include <limits.h>

/* -- static data ----------------------------------------------------------- */

/* configurable parameters (see timers_init) */
static int _utmrs_max_timeouts;

static utimer_t _utmrs_array[MAX_MICRO_TIMERS];
static utimer_t *_utmrs_free_list;
static utimer_t *_utmrs_active_list;

static int _utmrs_initialized = 0;
static utimer_id_t _utmrs_next_id = 0;

/** -- static function prototypes ------------------------------------------- */
static inline int utimers_cmp( utimer_t *a, utimer_t *b );
static int utimers_array_insert( utimer_t *timer );
static int utimers_array_remove( utimer_t *timer );

/** -- public functions ----------------------------------------------------- */
int utimers_is_initialized()
{ return _utmrs_initialized; }

int utimers_init(int max_simultaneous_timeouts)
{
    int i = MAX_MICRO_TIMERS - 1;

    _utmrs_free_list = NULL;
    _utmrs_active_list = NULL;

    while (0 <= i) {
        _utmrs_array[i].next = _utmrs_free_list;
        _utmrs_free_list = &_utmrs_array[i];
        -- i;
    }

    _utmrs_max_timeouts = max_simultaneous_timeouts;
    _utmrs_initialized = 1;

    return 0;
}

/* returns timer id, -1 on failure. */
utimer_id_t utimers_schedule(uticks_t dly, utimer_handler_t handler,
                             void *user_data)
{
    utimer_t timer;
    ASSERT(utimers_is_initialized());

    /* populate data structure */
    timer.id =  _utmrs_next_id ++;

    timer.base = micros();
    timer.dly = dly;
    timer.handler = handler;
    timer.user_data = user_data;

    return (0 == utimers_array_insert( &timer ))
        ? timer.id : -1;
}

/* returns number of microseconds before expiration, 0 if already
   expired, UINT_MAX if not found */
uticks_t utimers_timeleft(utimer_id_t id)
{
    utimer_t *head = _utmrs_active_list;
    ASSERT(utimers_is_initialized());

    while (NULL != head) {

        if (head->id == id) {
            uticks_t deadline = head->base + head->dly;
            uticks_t now = micros();
            return deadline > now
                ? deadline - now
                : 0;
        }

        head = head->next;
    } /* while */

    return UINT_MAX; /* not found */
}

int utimers_cancel(utimer_id_t id)
{
    utimer_t *head = _utmrs_active_list;
    ASSERT(utimers_is_initialized());

    while (NULL != head) {

        if (head->id == id) {
            int rc = utimers_array_remove(head);
            ASSERT (0 == rc);

            return 0;
        }

        head = head->next;
    } /* while */

    return -1; /* not found */
}

void utimers_check()
{
    int rc, count = _utmrs_max_timeouts;
    utimer_t *next, *head = _utmrs_active_list;

    ASSERT(utimers_is_initialized());

    uticks_t now = micros();

    while (NULL != head) {

        /* TODO: figure out clock condition */
        if (head->base + head->dly > now)
            break;

        next = head->next;

        if (! head->handler(head->id, now, head->user_data)) {
            rc = utimers_array_remove(head);
            ASSERT(0 == rc);
        }
        else {
            /* reschedule */
            head->base = now;
        }

        if (0 == -- count)
            break;

        head = next;
    } /* while */
}


/* -- static functions ------------------------------------------------------ */
static inline int utimers_cmp( utimer_t *a, utimer_t *b )
{
    uticks_t ta = a->base + a->dly;
    uticks_t tb = b->base + b->dly;

    return (ta <= tb) ? 1 : -1;
}

static int utimers_array_insert( utimer_t *timer )
{
    if (_utmrs_free_list == NULL)
        return -1;

    /* fetch head from free list */
    utimer_t *elem = _utmrs_free_list;
    _utmrs_free_list = elem->next;

    /* copy timer data  (avoid overlap) */
    if (elem != timer) {
        memcpy( elem, timer, sizeof(utimer_t));
    }

    /* sorted insertion */
    utimer_t *previous = NULL, *eye = _utmrs_active_list;
    while (NULL != eye && 0 < utimers_cmp( eye, elem)) {
        previous = eye;
        eye = eye->next;
    }

    if (NULL == previous) {
        _utmrs_active_list = elem;
    }
    else {
        previous->next = elem;
    }

    elem->next = eye;
    return 0;
}

static int timers_array_remove(utimer_t *timer)
{
    utimer_t *previous = NULL, *head = _utmrs_active_list;

    if (NULL == head)
        return -1;

    while (NULL != head && head != timer) {
        previous = head;
        head = head->next;
    }

    if (head != timer)
        return -1;

    if (NULL == previous) {
        _utmrs_active_list = head->next;
    }
    else {
        previous->next = head->next;
    }

    /* put block back into free list */
    timer->next = _utmrs_free_list;
    _utmrs_free_list = timer;

    return 0;
}
