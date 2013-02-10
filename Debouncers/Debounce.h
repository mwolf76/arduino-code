/**
 * @file Debounce.h
 * @brief Button debouncers library header file
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
#ifndef DEBOUNCE_H_DEFINED
#define DEBOUNCE_H_DEFINED

#include <Timers.h>

const int MAX_DEBOUNCERS = 10;
const int DEBOUNCE_DEFAULT_RESOLUTION = 10;
const int DEBOUNCE_DEFAULT_CLICK_TICKS = 10;
const int DEBOUNCE_DEFAULT_HOLD_TICKS  = 100;

/* -- custom typedefs ------------------------------------------------------- */
typedef short deb_id_t;

typedef enum {
    DEB_IDLE,
    DEB_WAIT,
    DEB_CLICK,
    DEB_HOLD,
} debouncer_state_t;

typedef int debounce_handler_t(deb_id_t id, debouncer_state_t state, void *ctx);

typedef struct debouncer_TAG {

    /** id */
    deb_id_t id;

    /** input pin to be debounced */
    short input;

    /** fsm */
    debouncer_state_t state;

    /** callback */
    debounce_handler_t *handler;

    /** reserved the user callback */
    void *user_data;

    /** current number of high samples */
    ticks_t ticks_high;

    /** number of high samples required for click */
    ticks_t ticks_click;

    /** number of high samples required for hold */
    ticks_t ticks_hold;

    struct debouncer_TAG *next;
} debouncer_t ;

/* -- public interface ------------------------------------------------------ */

/** initialize debounce library (call from setup) */
int debouncers_init(ticks_t resolution  = DEBOUNCE_DEFAULT_RESOLUTION,
                    ticks_t click_ticks = DEBOUNCE_DEFAULT_CLICK_TICKS,
                    ticks_t hold_ticks  = DEBOUNCE_DEFAULT_HOLD_TICKS);

/** returns true if lib is initialized, false otherwise. */
int debouncers_is_initialized();

/** arms a debouncer with user-provided data */
deb_id_t debouncers_enable(debounce_handler_t handler,
                           short input, void *user_data);

/** stops a debouncer with given id */
int debouncers_disable(deb_id_t id);

#endif
