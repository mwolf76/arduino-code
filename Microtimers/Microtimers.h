#ifndef MICROTIMERS_H_DEFINED
#define MICROTIMERS_H_DEFINED

#define MAX_MICROTIMERS (5)

/* -- custom typedefs ------------------------------------------------------- */

typedef short timer_id_t;
typedef unsigned long ticks_t;
typedef int timer_handler_t(timer_id_t id, ticks_t now, void *ctx);

typedef struct utimer_TAG {

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

    struct utimer_TAG *next;
} utimer_t;

/* -- public interface ------------------------------------------------------ */

/** returns true if lib is initialized, false otherwise */
int microtimers_is_initialized();

/** initializes the library. Must be invoked once, before using the library */
int microtimers_init();

/** schedules a delayed action. returns timer id if succesful, -1 otherwise */
timer_id_t microtimers_schedule(ticks_t dly, timer_handler_t handler,
                           void *user_data);

/** returns number of milliseconds before expiration */
ticks_t microtimers_timeleft(timer_id_t id);

/** cancels an existing timer. Returns 0 if succesful, -1 otherwise */
int microtimers_cancel(timer_id_t id);

/** to be invoked by main loop() */
void microtimers_check();

#endif
