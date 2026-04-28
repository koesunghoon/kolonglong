#ifndef _HW_DRIVER_TEMP_H_
#define _HW_DRIVER_TEMP_H_

#include "hw_def.h"

bool tempInit();
float tempReadAuto();
float tempReadSingle();

void tempStartAuto();
void tempStopAuto();

#endif // _HW_DRIVER_TEMP_H