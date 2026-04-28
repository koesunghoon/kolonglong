#ifndef _HW_DRIVER_BUTTON_H_
#define _HW_DRIVER_BUTTON_H_
#include "hw_def.h"

#define BUTTON_MAX_CH 1

void buttonInit(void);
void buttonEnable(bool enable);
bool buttonGetEnable(void);

#endif