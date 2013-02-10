/*
 * @file Timers.h
 * @brief Timers library header file
 *
 * Copyright (C) 2013 Marco Pensallorto < marco AT pensallorto DOT gmail DOT com >
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 **/
#ifndef TIMERS_H_DEFINED
#define TIMERS_H_DEFINED

const int MAX_TIMERS = 20;
const int TIMERS_DEFAULT_MAX_SIMULTANEOUS_TIMEOUTS = 5;

/* -- custom typedefs ------------------------------------------------------- */

typedef short timer_id_t;
typedef unsigned long ticks_t;
typedef int timer_handler_t(timer_id_t id, ticks_t now, void *ctx);

typedef struct timer_TAG {

    /** timer ID */
    timer_id_t id;

    /** delay (ms) */
    ticks_t dly;

    /** current time when timer was started */
    ticks_t base;

    /** Scheduled time action */
    timer_handler_t *handler;

    /** Reserved for the user */
    void *user_data;

    struct timer_TAG *next;
} timer_t;

/* -- public interface ------------------------------------------------------ */

/** returns true if lib is initialized, false otherwise */
int timers_is_initialized();

/** initializes the library. Must be invoked once, before using the library */
int timers_init(int max_simultaneous_timeouts =
                TIMERS_DEFAULT_MAX_SIMULTANEOUS_TIMEOUTS);

/** schedules a delayed action. returns timer id if succesful, -1 otherwise */
timer_id_t timers_schedule(ticks_t dly, timer_handler_t handler,
                           void *user_data);

/** returns number of milliseconds before expiration */
ticks_t timers_timeleft(timer_id_t id);

/** cancels an existing timer. Returns 0 if succesful, -1 otherwise */
int timers_cancel(timer_id_t id);

/** to be invoked by main loop() */
void timers_check();

#endif
