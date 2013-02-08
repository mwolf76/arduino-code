#include <Microtimers.h>
#include <Debug.h>

#include <stdio.h>
#include <string.h>
#include <limits.h>

/* static data */
static utimer_t microtimers[MAX_MICROTIMERS];
static utimer_t *microtimers_free_list;
static utimer_t *microtimers_active_list;

static int microtimers_initialized = 0;
static timer_id_t microtimers_next_id = 0;

/** -- static function prototypes ------------------------------------------- */
static inline int microtimers_cmp( utimer_t *a, utimer_t *b );
static int microtimers_array_insert( utimer_t *timer );
static int microtimers_array_remove( utimer_t *timer );

/** -- public functions ----------------------------------------------------- */
int microtimers_is_initialized()
{ return microtimers_initialized; }

int microtimers_init()
{
    int i = MAX_MICROTIMERS - 1;

    microtimers_free_list = NULL;
    microtimers_active_list = NULL;

    while (0 <= i) {
        microtimers[i].next = microtimers_free_list;
        microtimers_free_list = &microtimers[i];
        -- i;
    }

    microtimers_initialized = 1;
    return 0;
}

/* returns timer id, -1 on failure. */
timer_id_t microtimers_schedule( ticks_t dly, timer_handler_t handler,
                            void *user_data)
{
    utimer_t timer;
    ASSERT(microtimers_is_initialized());

    /* populate data structure */
    timer.id =  microtimers_next_id ++;

    timer.base = micros();
    timer.dly = dly;
    timer.handler = handler;
    timer.user_data = user_data;

    return (0 == microtimers_array_insert( &timer ))
        ? timer.id : -1;
}

/* returns number of microseconds before expiration, 0 if already
   expired, UINT_MAX if not found */
ticks_t microtimers_timeleft(timer_id_t id)
{
    utimer_t *head = microtimers_active_list;
    ASSERT(microtimers_is_initialized());

    while (NULL != head) {

        if (head->id == id) {
            ticks_t deadline = head->base + head->dly;
            ticks_t now = micros();
            return deadline > now
                ? deadline - now
                : 0;
        }

        head = head->next;
    } /* while */

    return UINT_MAX; /* not found */
}

int microtimers_cancel(timer_id_t id)
{
    utimer_t *head = microtimers_active_list;
    ASSERT(microtimers_is_initialized());

    while (NULL != head) {

        if (head->id == id) {
            int rc = microtimers_array_remove(head);
            ASSERT (0 == rc);

            return 0;
        }

        head = head->next;
    } /* while */

    return -1; /* not found */
}

void microtimers_check()
{
    int rc;
    utimer_t *next, *head = microtimers_active_list;

    ASSERT(microtimers_is_initialized());

    ticks_t now = micros();

    while (NULL != head) {

        /* TODO: figure out clock condition */
        if (head->base + head->dly > now)
            break;

        next = head->next;

        if (! head->handler(head->id, now, head->user_data)) {
            rc = microtimers_array_remove(head);
            ASSERT(0 == rc);
        }
        else {
            /* reschedule */
            head->base = micros();
        }

        // if (0 == -- count) (microtimers allow for just one timeout
        break;

        head = next;
    } /* while */
}


/* -- static functions ------------------------------------------------------ */
static inline int microtimers_cmp( utimer_t *a, utimer_t *b )
{
    ticks_t ta = a->base + a->dly;
    ticks_t tb = b->base + b->dly;

    return (ta <= tb) ? 1 : -1;
}

static int microtimers_array_insert( utimer_t *timer )
{
    if (microtimers_free_list == NULL)
        return -1;

    /* fetch head from free list */
    utimer_t *elem = microtimers_free_list;
    microtimers_free_list = elem->next;

    /* copy timer data  (avoid overlap) */
    if (elem != timer) {
        memcpy( elem, timer, sizeof(utimer_t));
    }

    /* sorted insertion */
    utimer_t *previous = NULL, *eye = microtimers_active_list;
    while (NULL != eye && 0 < microtimers_cmp( eye, elem)) {
        previous = eye;
        eye = eye->next;
    }

    if (NULL == previous) {
        microtimers_active_list = elem;
    }
    else {
        previous->next = elem;
    }

    elem->next = eye;
    return 0;
}

static int microtimers_array_remove(utimer_t *timer)
{
    utimer_t *previous = NULL, *head = microtimers_active_list;

    if (NULL == head)
        return -1;

    while (NULL != head && head != timer) {
        previous = head;
        head = head->next;
    }

    if (head != timer)
        return -1;

    if (NULL == previous) {
        microtimers_active_list = head->next;
    }
    else {
        previous->next = head->next;
    }

    /* put block back into free list */
    timer->next = microtimers_free_list;
    microtimers_free_list = timer;

    return 0;
}
