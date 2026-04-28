#ifndef __HW_DRIVER_LED_C__
#define __HW_DRIVER_LED_C__

#include "hw_def.h"

void ledInit(void);
void ledOn(void);
void ledOff(void);
void ledToggle(void);
bool ledGetStatus(void);

#endif // __HW_DRIVER_LED_C__