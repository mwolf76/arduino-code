#include <Timers.h>
#include <Debounce.h>
#include <Debug.h>
#include <Arduino.h>

/* -- static data ----------------------------------------------------------- */

/* configurable parameters (see debouncers_init) */
static ticks_t _debs_resolution;
static ticks_t _debs_click_ticks;
static ticks_t _debs_hold_ticks;

static debouncer_t _debs_array[MAX_DEBOUNCERS];
static debouncer_t *_debs_free_list;
static debouncer_t *_debs_active_list;

static int _debs_initialized = 0;
static deb_id_t _debs_next_id = 0;

/* -- static function prototypes -------------------------------------------- */
static int debouncers_check (timer_id_t unused, ticks_t now, void *data);
static int debouncers_fsm (debouncer_t *debouncer, int input);
static int debouncers_array_insert( debouncer_t *debouncer );

/* -- public functions ------------------------------------------------------ */
int debouncers_is_initialized()
{ return _debs_initialized; }

int debouncers_init(ticks_t resolution, ticks_t click_ticks, ticks_t hold_ticks)
{
    int i = MAX_DEBOUNCERS - 1;

    _debs_free_list = NULL;
    _debs_active_list = NULL;

    while (0 <= i) {
        memset(_debs_array + i, 0, sizeof(debouncer_t));
        _debs_array[i].next = _debs_free_list;
        _debs_free_list = &_debs_array[i];

        -- i;
    }

    /* setup static data */
    _debs_resolution = resolution;
    ASSERT(0 < _debs_resolution);

    _debs_click_ticks = click_ticks;
    ASSERT(0 < click_ticks);

    _debs_hold_ticks = hold_ticks;
    ASSERT(0 < _debs_hold_ticks);

    timers_schedule( _debs_resolution, debouncers_check, NULL );

    _debs_initialized = 1;

    return 0;
}

deb_id_t debouncers_enable( debounce_handler_t *handler,
                            short input, void *user_data)
{
    debouncer_t debouncer;
    ASSERT (debouncers_is_initialized());

    /* populate data structure */
    debouncer.id =  _debs_next_id ++;

    debouncer.state = DEB_IDLE;
    debouncer.input = input;
    debouncer.handler = handler;
    debouncer.user_data = user_data;

    debouncer.ticks_click = _debs_click_ticks;
    debouncer.ticks_hold = _debs_hold_ticks;

    return !debouncers_array_insert( &debouncer )
        ? debouncer.id : -1;
}

/* -- static functions ------------------------------------------------------ */

/* (reserved) this is used as a callback with Timers library */
static int debouncers_check (timer_id_t unused, ticks_t now, void *data)
{
    debouncer_t *head = _debs_active_list;
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
    if (_debs_free_list == NULL)
        return -1;

    /* fetch head from free list */
    debouncer_t *elem = _debs_free_list;
    _debs_free_list = _debs_free_list->next;

    /* copy debouncer data */
    memcpy( elem, debouncer, sizeof(debouncer_t));

    /* head insertion */
    elem->next = _debs_active_list;
    _debs_active_list = elem;

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
