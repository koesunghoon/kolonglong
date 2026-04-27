#ifndef _HW_DRIVER_PCA9685_H_
#define _HW_DRIVER_PCA9685_H_

#include "hw_def.h"

#define PCA9685_ADDR  (0x40 << 1)  // 0x80
#define REG_MODE1         0x00
#define REG_PRESCALE      0xFE
#define REG_LED0_ON_L     0x06
#define REG_MODE2    0x01

bool pca9685Init(void);
void pca9685SetAngle(uint8_t ch, float angle);

#endif