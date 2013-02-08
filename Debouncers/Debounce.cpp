#include <Timers.h>
#include <Debounce.h>
#include <Debug.h>
#include <Arduino.h>

/* -- static data ----------------------------------------------------------- */
static debouncer_t debouncers[MAX_DEBOUNCERS];
static debouncer_t *debouncers_free_list;
static debouncer_t *debouncers_active_list;

static int debouncers_initialized = 0;
static deb_id_t debouncers_next_id = 0;

/* -- static function prototypes -------------------------------------------- */
static int debouncers_check (timer_id_t unused, ticks_t now, void *data);

static int debouncers_fsm (debouncer_t *debouncer, int input);
static int debouncers_array_insert( debouncer_t *debouncer );

/* -- public functions ------------------------------------------------------ */

int debouncers_is_initialized()
{ return debouncers_initialized; }

int debouncers_init()
{
    int i = MAX_DEBOUNCERS - 1;

    debouncers_free_list = NULL;
    debouncers_active_list = NULL;

    while (0 <= i) {
        memset(debouncers + i, 0, sizeof(debouncer_t));
        debouncers[i].next = debouncers_free_list;
        debouncers_free_list = &debouncers[i];

        -- i;
    }

    /* uses Timers as a subsystem. Paranoid check */
    ASSERT(timers_is_initialized());
    timers_schedule( DEBOUNCE_RESOLUTION, debouncers_check, NULL );

    debouncers_initialized = 1;
    return 0;
}

deb_id_t debouncers_enable( debounce_handler_t *handler,
                            short input, void *user_data)
{
    debouncer_t debouncer;
    ASSERT (debouncers_is_initialized());

    /* populate data structure */
    debouncer.id =  debouncers_next_id ++;

    debouncer.state = DEB_IDLE;
    debouncer.input = input;
    debouncer.handler = handler;
    debouncer.user_data = user_data;

    debouncer.ticks_hold = DEBOUNCE_HOLD_TICKS; // TODO: param?
    debouncer.ticks_click = DEBOUNCE_CLICK_TICKS; // TODO: param?

    return !debouncers_array_insert( &debouncer )
        ? debouncer.id : -1;
}

/* -- static functions ------------------------------------------------------ */

/* (reserved) this is used as a callback with Timers library */
static int debouncers_check (timer_id_t unused, ticks_t now, void *data)
{
    debouncer_t *head = debouncers_active_list;
    ASSERT (debouncers_is_initialized());

    while (NULL != head) {
        const int button = (HIGH == digitalRead(head->input));

#if ! USE_SLCD
        char buf[200];
        snprintf( buf, 200, "ID = %d (in %d), state = %s, user_data = %p",
                  head->id, head->input, button ? "DOWN" : "UP",
                  head->user_data );
        Serial.println(buf);
#endif

        if (debouncers_fsm(head, button)) {
            debounce_handler_t *handler = head->handler;

            /** @TODO do something with return code? */
            handler(head->id, head->state, head->user_data);
        }

        head = head->next;
    } /* while */

    return 1; /* infinite rescheduling */
}

/* arms a new debouncer handler: returns -1 on error, 0 otherwise */
static int debouncers_array_insert( debouncer_t *debouncer )
{
    if (debouncers_free_list == NULL)
        return -1;

    /* fetch head from free list */
    debouncer_t *elem = debouncers_free_list;
    debouncers_free_list = debouncers_free_list->next;

    /* copy debouncer data */
    memcpy( elem, debouncer, sizeof(debouncer_t));

    /* head insertion */
    elem->next = debouncers_active_list;
    debouncers_active_list = elem;

    return 0;
}

/* returns 1 if an event is triggered, 0 otherwise */
static int debouncers_fsm (debouncer_t *debouncer, int input)
{
    if (! input) {
        debouncer->ticks_high = 0;
        debouncer->state = DEB_IDLE;
        return 0;
    }

    ++ debouncer->ticks_high;

    /* FSM transition relation */
    switch(debouncer->state) {

    case DEB_IDLE:
        debouncer->state = DEB_WAIT;
        break;

    case DEB_WAIT:
        if (debouncer->ticks_high >= debouncer->ticks_click ) {
            debouncer->state = DEB_CLICK;
            return 1;
        }
        break;

    case DEB_CLICK:
        if (debouncer->ticks_high >= debouncer->ticks_hold ) {
            debouncer->state = DEB_HOLD;
            debouncer->ticks_high = 0;
            return 1;
        }
        break;

    case DEB_HOLD:
        if (debouncer->ticks_high >= debouncer->ticks_hold ) {
            debouncer->ticks_high = 0;
            return 1;
        }
        break;

    default:
        HALT(); /* unexpected */
    }

    return 0;
}
