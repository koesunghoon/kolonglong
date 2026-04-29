#ifndef _HW_DRIVER_PCA9685_H_
#define _HW_DRIVER_PCA9685_H_
#include "cmsis_os.h"  // osDelay 사용을 위해

#include "hw_def.h"

#define PCA9685_ADDR  (0x40 << 1)  // 0x80
#define REG_MODE1         0x00
#define REG_PRESCALE      0xFE
#define REG_LED0_ON_L     0x06
#define REG_MODE2    0x01

bool pca9685Init(void);
void pca9685SetAngle(uint8_t ch, float angle);

void pca9685SetAngleSmooth(uint8_t ch, float target_angle, uint16_t duration_ms);

void pca9685SetAngleSmoothDual(uint8_t ch0, float target0, uint8_t ch1, float target1, uint16_t duration_ms);

#endif