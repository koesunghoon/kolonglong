#ifndef __AP_AP_H_
#define __AP_AP_H_  
#include "def.h"
#include "hw_def.h"
#include "bsp.h"
#include "hw.h"
#include "monitor.h"
#include "conveyor_belt.h"

void apInit(void);
void apMain(void);
void apStopAutoTask(void);

#endif //__AP_AP_H_