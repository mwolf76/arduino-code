#ifndef DEBOUNCE_H_DEFINED
#define DEBOUNCE_H_DEFINED

#define MAX_DEBOUNCERS (10)
#define DEBOUNCE_RESOLUTION (10)

#define DEBOUNCE_CLICK_TICKS (10)
#define DEBOUNCE_HOLD_TICKS (100)

typedef enum {
    DEB_IDLE,
    DEB_WAIT,
    DEB_CLICK,
    DEB_HOLD,
} debouncer_state_t;

/** debouncer handler function type */
typedef short deb_id_t;

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
int debouncers_init();

/** returns true if lib is initialized, false otherwise. */
int debouncers_is_initialized();

/** arms a debouncer with user-provided data */
deb_id_t debouncers_enable(debounce_handler_t handler,
                           short input, void *user_data);

/** stops a debouncer with given id */
int debouncers_disable(deb_id_t id);

#endif
