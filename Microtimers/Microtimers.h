/**
 * @file Microtimers.h
 * @brief Microtimers library header file
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
#ifndef uTIMERS_H_DEFINED
#define uTIMERS_H_DEFINED

const int MAX_MICRO_TIMERS = 20;
const int MICRO_TIMERS_DEFAULT_MAX_SIMULTANEOUS_TIMEOUTS = 1;

/* -- custom typedefs ------------------------------------------------------- */

typedef short utimer_id_t;
typedef unsigned long uticks_t;
typedef int utimer_handler_t(utimer_id_t id, uticks_t now, void *ctx);

typedef struct timer_TAG {

    /** timer ID */
    utimer_id_t id;

    /** delay (ms) */
    uticks_t dly;

    /** current time when timer was started */
    uticks_t base;

    /** Scheduled time action */
    utimer_handler_t *handler;

    /** Reserved for the user */
    void *user_data;

    struct timer_TAG *next;
} utimer_t;

/* -- public interface ------------------------------------------------------ */

/** returns true if lib is initialized, false otherwise */
int utimers_is_initialized();

/** initializes the library. Must be invoked once, before using the library */
int utimers_init(int max_simultaneous_timeouts =
                 MICRO_TIMERS_DEFAULT_MAX_SIMULTANEOUS_TIMEOUTS);

/** schedules a delayed action. returns timer id if succesful, -1 otherwise */
utimer_id_t utimers_schedule(uticks_t dly, utimer_handler_t handler,
                             void *user_data);

/** returns number of milliseconds before expiration */
uticks_t utimers_timeleft(utimer_id_t id);

/** cancels an existing timer. Returns 0 if succesful, -1 otherwise */
int utimers_cancel(utimer_id_t id);

/** to be invoked by main loop() */
void utimers_check();

#endif
