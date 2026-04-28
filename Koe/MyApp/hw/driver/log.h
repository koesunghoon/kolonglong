#ifndef _HW_DRIVER_LOG_H_
#define _HW_DRIVER_LOG_H_

#include "def.h"
#include "hw_def.h"
#include "log_def.h"
#include "uart.h"
#include "cli.h"


bool logInit();
void logSetLevel(uint8_t level);
uint8_t logGetLevel();
void clilog(uint8_t argc, char** argv);



#endif //_HW_DRIVER_LOG_H_