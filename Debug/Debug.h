#ifndef DEBUG_H_DEFINED
#define DEBUG_H_DEFINED

#include <Arduino.h>

#define ASSERT(cond) if (!(cond)) do {                                  \
            digitalWrite(debug_error(), HIGH);                          \
            abort();                                                    \
        } while(0)

#define HALT() do {                                                     \
        digitalWrite(debug_error(), HIGH);                              \
        abort();                                                        \
    } while(0)

#define FLASH() do {                                                    \
        digitalWrite(debug_error(), HIGH);                              \
        delay(1000);                                                    \
        digitalWrite(debug_error(), LOW);                               \
        delay(1000);                                                    \
    } while(0)

#define FLASH_TWICE()                                                   \
    do { FLASH(); FLASH(); } while (0)

#define HERE() do {                                                     \
        snprintf(serial_msg, SERIAL_MSG_SIZE,                           \
                 "%s %s %d", __FILE__, __func__, __LINE__);             \
        Serial.println(serial_msg);                                     \
    } while (0)

/** returns 0 */
int debug_init();

/** error LED */
int debug_error();

#endif
