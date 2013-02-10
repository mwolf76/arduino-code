#include <math.h>

#include <Timers.h>

#include <Debounce.h>
#include <Debug.h>

#include <SerialLCD.h>
#include <SoftwareSerial.h>

#define N_SAMPLES (24)

#define TEMP_SAMPLE_PERIOD (125)
#define LCD_UPDATE_PERIOD  (500)
#define ACT_UPDATE_PERIOD (1000)
#define CLK_PERIOD        (1000)

/* SLCD and Serial monitor can not be used at once, uncomment
   following line to enable SLCD sub-system. */
#define USE_SLCD 1

/* -- Helpers --------------------------------------------------------------- */
#define PRINT_TEMP(temp)                                                \
    do {                                                                \
        SLCDprintFloat((temp), 1);                                      \
    } while (0)

#define PRINT_TIME(h,m)                                                 \
    do {                                                                \
    } while (0)

#define GOTO_XY(x,y)                                                    \
    do { slcd.setCursor((x), (y)); } while (0)

#define CLEAR_LINE(line)                                                \
    do {                                                                \
        slcd.setCursor(0, (line));                                      \
        slcd.print("               ");                                  \
    } while (0)

/* -- pin assignments ------------------------------------------------------- */
const int ai_thermistor = 0;

// const int di_increment = 7;
// const int di_decrement = 8;

const int di_clk_switch = 7;
const int di_clk_adjust = 8;

const int do_actuate = 4;

#if USE_SLCD
const int slcd_tx = 11;
const int slcd_rx = 12;
SerialLCD slcd(slcd_tx, slcd_rx);
#endif

typedef enum {
    H_LOW,
    H_HIGH,
} hysteresis_t;

typedef enum {
    CTL_RUNNING,
    CTL_SET_HOUR,
    CTL_SET_MINUTE,
} ctl_t;

struct tm {
    int tm_sec;         /* seconds after the minute [0-60] */
    int tm_min;         /* minutes after the hour [0-59] */
    int tm_hour;        /* hours since midnight [0-23] */
};

typedef struct {
    double samples[N_SAMPLES];
    unsigned short last_sample;

    int heartbeat;
    int initialized;

    double curr_temperature;
    double goal_temperature;

    double hyst_offset;
    hysteresis_t  hyst_status;

    ctl_t ctl;
    struct tm now;
} display_ctx_t;

/* temperature contexts (used in several different handlers) */
display_ctx_t display_ctx;

typedef struct {
    double increment;
    double limit;

    double *pgoal_temperature;
} deb_ctx_t;

/* debouncer contexts */
deb_ctx_t increment_ctx;
deb_ctx_t decrement_ctx;

/* -- static function prototypes -------------------------------------------- */

/* timed action callbacks  */
static int display_callback(timer_id_t unused, ticks_t now, void *ctx);
static int sampling_callback(timer_id_t unused, ticks_t now, void *ctx);
static int thermal_callback(timer_id_t unused, ticks_t now, void *ctx);
static int clock_callback(timer_id_t unused, ticks_t now, void *ctx);

/* button callbacks */
static int thermal_button_callback(deb_id_t unused, debouncer_state_t state,
                                   void *ctx);

static int clk_switch_callback(deb_id_t unused, debouncer_state_t state,
                               void *ctx);

static int clk_adjust_callback(deb_id_t unused, debouncer_state_t state,
                               void *ctx);


/* command function, with actuates to the outer world. */
static int control(int status);

/* LCD helpers */
static void update_display(display_ctx_t *pctx);

/* misc */
static double readTemp();
static void SLCDprintFloat(double number, uint8_t digits);

/** -- implementation ------------------------------------------------------- */
void setup()
{
    int rc;

    // pinMode(di_increment, INPUT);
    // pinMode(di_decrement, INPUT);
    pinMode(di_clk_adjust, INPUT);
    pinMode(di_clk_switch, INPUT);
    pinMode(do_actuate, OUTPUT);

    debug_init();

#if USE_SLCD
    slcd.begin();
#else
    Serial.begin(9600); /* debug only */
#endif

    /* -- data initialization ----------------------------------------------- */
    memset( &display_ctx, 0, sizeof(display_ctx_t));
    display_ctx.hyst_offset = .2 ;
    display_ctx.hyst_status = H_LOW;
    display_ctx.goal_temperature = 25.0;
    display_ctx.ctl = CTL_RUNNING;

    memset( &increment_ctx, 0, sizeof(deb_ctx_t));
    increment_ctx.increment = .5;
    increment_ctx.limit = 40.0;
    increment_ctx.pgoal_temperature = &display_ctx.goal_temperature;

    memset( &decrement_ctx, 0, sizeof(deb_ctx_t));
    decrement_ctx.increment = - .5;
    decrement_ctx.limit = 0.0;
    decrement_ctx.pgoal_temperature = &display_ctx.goal_temperature;

    /* -- timers ------------------------------------------------------------ */
    rc = timers_init();
    if (0 != rc) HALT();

    rc = timers_schedule(TEMP_SAMPLE_PERIOD, sampling_callback, &display_ctx);
    if (0 > rc) HALT();

    rc = timers_schedule(LCD_UPDATE_PERIOD, display_callback, &display_ctx);
    if (0 > rc) HALT();

    rc = timers_schedule(ACT_UPDATE_PERIOD, thermal_callback, &display_ctx);
    if (0 > rc) HALT();

    rc = timers_schedule(CLK_PERIOD, clock_callback, &display_ctx);
    if (0 > rc) HALT();

    /* -- debouncers  ------------------------------------------------------- */
    rc = debouncers_init();
    if (0 != rc) HALT();

    // rc = debouncers_enable( thermal_button_callback, di_increment,
    // &increment_ctx);
    // if (0 > rc) HALT();

    // rc = debouncers_enable( thermal_button_callback, di_decrement,
    // &decrement_ctx);
    // if (0 > rc) HALT();

    rc = debouncers_enable( clk_switch_callback, di_clk_switch, &display_ctx);
    if (0 > rc) HALT();

    rc = debouncers_enable( clk_adjust_callback, di_clk_adjust, &display_ctx);
    if (0 > rc) HALT();
}

void loop()
{
    timers_check();
}

/* -- static functions ------------------------------------------------------ */
static int thermal_button_callback(deb_id_t unused, debouncer_state_t state,
                                   void *ctx)
{
    deb_ctx_t *pctx = (deb_ctx_t *) ctx;
    double tmp = *(pctx->pgoal_temperature) + pctx->increment;

    if (0 < pctx->increment) {
        /* raising, still within acceptable limits */
        if (tmp <= pctx->limit) {
            * pctx->pgoal_temperature = tmp;
            return 0;
        }
    }
    else {
        /* lowering, still within acceptable limits */
        if (pctx->limit <= tmp) {
            * pctx->pgoal_temperature = tmp;
            return 0;
        }
    }

    return -1; /* rejected */
}

static int clk_switch_callback(deb_id_t unused, debouncer_state_t state,
                               void *ctx)
{
    display_ctx_t *pctx = (display_ctx_t *) ctx;

    switch (pctx->ctl) {
    case CTL_RUNNING:
        pctx->ctl = CTL_SET_HOUR;
        break;

    case CTL_SET_HOUR:
        pctx->ctl = CTL_SET_MINUTE;
        break;

    case CTL_SET_MINUTE:
        pctx->ctl = CTL_RUNNING;
        break;

    default: HALT();
    }

    return 0;
}

static int clk_adjust_callback(deb_id_t unused, debouncer_state_t state,
                               void *ctx)
{
    display_ctx_t *pctx = (display_ctx_t *) ctx;

    switch (pctx->ctl) {
    case CTL_RUNNING:
        /* nop */
        break;

    case CTL_SET_HOUR:
        if (24 <= ++ pctx->now.tm_hour) {
            pctx->now.tm_hour -= 24;
        }
        break;

    case CTL_SET_MINUTE:
        if (60 <= ++ pctx->now.tm_min) {
            pctx->now.tm_min -= 60;
        }
        break;

    default: HALT();
    }

    return 0;
}


static int display_callback(timer_id_t unused, ticks_t now, void *ctx)
{
#if USE_SLCD
    display_ctx_t *pctx = (display_ctx_t *) ctx;

    if (! pctx->initialized) {
        static int count = 0;
        const int ndots = 3;

        const char *msg = "Initializing";

        if (!count) {
            slcd.setCursor(0, 0);
            slcd.print (msg);
        } else slcd.setCursor(strlen(msg), 0);

        for (int i = 0; i < ndots; ++ i) {
            slcd.print ( i <= count ? '.' : ' ' );
        }

        if ( ndots == ++ count ) count = 0;
    }
    else {
        /* clear line, and build frame only once */
        static int once = 0;
        if (!once) {
            CLEAR_LINE(0);
            for (int i = 0; i < 2; ++ i) {
                slcd.setCursor(5, i);
                slcd.print('C');
            }
            once = 1;
        }

        update_display(pctx);
    }
#endif

    return 1; /* infinite rescheduling */
}

static int thermal_callback(timer_id_t unused, ticks_t now, void *ctx)
{
    display_ctx_t *pctx = (display_ctx_t *) ctx;
    if (! pctx->initialized) return 1;

    /* turn on/off? */
    if (H_LOW == pctx->hyst_status) {
        double thr = pctx->goal_temperature - pctx->hyst_offset;
        if (pctx->curr_temperature < thr) {
            pctx->hyst_status = H_HIGH;
            control(1);
        }
    }
    else if (H_HIGH == pctx->hyst_status) {
        double thr = pctx->goal_temperature + pctx->hyst_offset;
        if (thr < pctx->curr_temperature) {
            pctx->hyst_status = H_LOW;
            control(0);
        }
    }
    else HALT();

    return 1; /* infinite rescheduling */
}

static int sampling_callback(timer_id_t unused, ticks_t now, void *ctx)
{
    display_ctx_t *pctx = (display_ctx_t *) ctx;

    /* read a sample into ring buffer */
    pctx->samples[pctx->last_sample ++] = readTemp();
    if (N_SAMPLES == pctx->last_sample) {
        pctx->initialized = 1;
        pctx->last_sample = 0;
    }

    if (pctx->initialized) {
        double sum = .0;
        int i = 0;

        for (i = 0; i < N_SAMPLES; ++ i) {
            sum += pctx->samples[i];
        }
        pctx->curr_temperature = sum / N_SAMPLES;
    }

    return 1; /* infinite rescheduling */
}

static int clock_callback(timer_id_t unused, ticks_t now, void *ctx)
{
    display_ctx_t *pctx = (display_ctx_t *) ctx;

    ++ pctx->now.tm_sec;
    if (60 <= pctx->now.tm_sec) {
        pctx->now.tm_sec -= 60;

        ++ pctx->now.tm_min;
        if (60 <= pctx->now.tm_min) {
            pctx->now.tm_min -= 60;

            ++ pctx->now.tm_hour;
            if (24 <= pctx->now.tm_hour) {
                pctx->now.tm_hour -= 24;
            }
        }
    }

    return 1; /* infinite rescheduling */
}

static void update_display(display_ctx_t *pctx)
{
#if USE_SLCD
    GOTO_XY(0, 0);
    PRINT_TEMP(pctx->curr_temperature);

    GOTO_XY(15, 0);
    pctx->heartbeat = ! pctx->heartbeat;
    slcd.print(pctx->heartbeat ? '*' : ' ' );

    GOTO_XY(0, 1);
    PRINT_TEMP(pctx->goal_temperature);

    GOTO_XY(11, 1);
    {
        char buf[10];

        if (CTL_RUNNING == pctx->ctl) {
            snprintf(buf, 10, "%02d:%02d", pctx->now.tm_hour, pctx->now.tm_min);
        }
        else if (CTL_SET_HOUR == pctx->ctl) {
            if (pctx->heartbeat) {
                snprintf(buf, 10, "%02d:%02d",
                         pctx->now.tm_hour, pctx->now.tm_min);
            }
            else {
                snprintf(buf, 10, "  :%02d", pctx->now.tm_min);
            }
        }
        else if (CTL_SET_MINUTE == pctx->ctl) {
            if (pctx->heartbeat) {
                snprintf(buf, 10, "%02d:%02d",
                         pctx->now.tm_hour, pctx->now.tm_min);
            }
            else {
                snprintf(buf, 10, "%02d:  ", pctx->now.tm_hour);
            }
        }

        slcd.print(buf);
    }


    PRINT_TIME(pctx->now.tm_hour, pctx->now.tm_min);
#endif
}

static inline int control(int status)
{
    digitalWrite( do_actuate, status ? HIGH : LOW );
    return 0;
}

/* -- helpers --------------------------------------------------------------- */
#if USE_SLCD
static void SLCDprintFloat(double number, uint8_t digits)
{
    // Handle negative numbers
    if (number < 0.0) {
        slcd.print('-');
        number = -number;
    }

    // Round correctly so that slcd.print(1.999, 2) prints as "2.00"
    double rounding = 0.5;
    for (uint8_t i=0; i<digits; ++i) {
        rounding /= 10.0;
    }

    number += rounding;

    // Extract the integer part of the number and print it
    unsigned long int_part = (unsigned long)number;
    float remainder = number - (float)int_part;
    slcd.print(int_part , DEC); // base DEC

    // Print the decimal point, but only if there are digits beyond
    if (digits > 0) {
        slcd.print(".");
    }

    // Extract digits from the remainder one at a time
    while (digits-- > 0) {
        remainder *= 10.0;
        float toPrint = float(remainder);
        slcd.print(toPrint , DEC);//base DEC
        remainder -= toPrint;
    }
}
#endif

static double readTemp()
{
    const int B=3975;
    double res, sensor;
    int sensorValue = analogRead( ai_thermistor );

    sensor=(double)( 1023 - sensorValue ) * 10000 / sensorValue;
    res =1 / (log( sensor / 10000 ) / B + 1 / 298.15 ) - 273.15;
    return res;
}
