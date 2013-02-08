#include <Timers.h>
#include <Debug.h>

#include <stdio.h>
#include <string.h>
#include <limits.h>

/* static data */
static timer_t timers[MAX_TIMERS];
static timer_t *timers_free_list;
static timer_t *timers_active_list;

static int timers_initialized = 0;
static timer_id_t timers_next_id = 0;

/** -- static function prototypes ------------------------------------------- */
static inline int timers_cmp( timer_t *a, timer_t *b );
static int timers_array_insert( timer_t *timer );
static int timers_array_remove( timer_t *timer );

/** -- public functions ----------------------------------------------------- */
int timers_is_initialized()
{ return timers_initialized; }

int timers_init()
{
    int i = MAX_TIMERS - 1;

    timers_free_list = NULL;
    timers_active_list = NULL;

    while (0 <= i) {
        timers[i].next = timers_free_list;
        timers_free_list = &timers[i];
        -- i;
    }

    timers_initialized = 1;
    return 0;
}

/* returns timer id, -1 on failure. */
timer_id_t timers_schedule( ticks_t dly, timer_handler_t handler,
                            void *user_data)
{
    timer_t timer;
    ASSERT(timers_is_initialized());

    /* populate data structure */
    timer.id =  timers_next_id ++;

    timer.base = millis();
    timer.dly = dly;
    timer.handler = handler;
    timer.user_data = user_data;

    return (0 == timers_array_insert( &timer ))
        ? timer.id : -1;
}

/* returns number of milliseconds before expiration, 0 if already
   expired, UINT_MAX if not found */
ticks_t timers_timeleft(timer_id_t id)
{
    timer_t *head = timers_active_list;
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
    timer_t *head = timers_active_list;
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
    int rc, count = MAX_SIMULTANEOUS_TIMEOUTS;
    timer_t *next, *head = timers_active_list;

    ASSERT(timers_is_initialized());

    ticks_t now = millis();

    while (NULL != head) {

        /* TODO: figure out clock condition */
        if (head->base + head->dly > now)
            break;

        next = head->next;

        if (! head->handler(head->id, now, head->user_data)) {
            rc = timers_array_remove(head);
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
static inline int timers_cmp( timer_t *a, timer_t *b )
{
    ticks_t ta = a->base + a->dly;
    ticks_t tb = b->base + b->dly;

    return (ta <= tb) ? 1 : -1;
}

static int timers_array_insert( timer_t *timer )
{
    if (timers_free_list == NULL)
        return -1;

    /* fetch head from free list */
    timer_t *elem = timers_free_list;
    timers_free_list = elem->next;

    /* copy timer data  (avoid overlap) */
    if (elem != timer) {
        memcpy( elem, timer, sizeof(timer_t));
    }

    /* sorted insertion */
    timer_t *previous = NULL, *eye = timers_active_list;
    while (NULL != eye && 0 < timers_cmp( eye, elem)) {
        previous = eye;
        eye = eye->next;
    }

    if (NULL == previous) {
        timers_active_list = elem;
    }
    else {
        previous->next = elem;
    }

    elem->next = eye;
    return 0;
}

static int timers_array_remove(timer_t *timer)
{
    timer_t *previous = NULL, *head = timers_active_list;

    if (NULL == head)
        return -1;

    while (NULL != head && head != timer) {
        previous = head;
        head = head->next;
    }

    if (head != timer)
        return -1;

    if (NULL == previous) {
        timers_active_list = head->next;
    }
    else {
        previous->next = head->next;
    }

    /* put block back into free list */
    timer->next = timers_free_list;
    timers_free_list = timer;

    return 0;
}
