#ifndef __AP_AP_H_
#define __AP_AP_H_

#include "def.h"
#include "hw_def.h"
#include "bsp.h"
#include "hw.h"
#include "monitor.h"
#include "pca9685.h"
#include "conveyor_belt.h"
#include "hx711.h"

void apInit(void);
void apMain(void);
void apStopAutoTask(void);
void armSystemTask(void *argument);
void vTaskLoadCell(void *argument);

#endif //__AP_AP_H_