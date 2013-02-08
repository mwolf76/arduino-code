#include <Debug.h>

/* reserved */
const int do_error = 13;

int debug_init()
{
    /* reserved for DEBUG */
    pinMode(do_error, OUTPUT);
    FLASH();
}

int debug_error()
{
    return do_error;
}
