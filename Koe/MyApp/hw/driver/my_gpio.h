#ifndef _HW_DRIER_MY_GPIO_H_
#define _HW_DRIER_MY_GPIO_H_
#include "hw_def.h"
#include <stdint.h>

//port number : 0=A, 1=B ....7=H
bool gpioExtWrite(uint8_t port_idx, uint8_t pin_num, uint8_t state);
int8_t gpioExtRead(uint8_t port_idx, uint8_t pin_num);


#endif
