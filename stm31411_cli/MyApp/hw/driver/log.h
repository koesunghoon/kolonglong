#ifndef _HW_DRIVER_LOG_H_
#define _HW_DRIVER_LOG_H_

#include "def.h"
#include "hw_def.h"
#include "log_def.h"
#include "uart.h"

bool logInit();
void logSetLevel(uint8_t level);
void cliLog(uint8_t argc, char** argv);
uint8_t logGetLevel();

#endif // _HW_DRIVER_LOG_H_