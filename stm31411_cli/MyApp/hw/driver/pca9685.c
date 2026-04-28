#include "pca9685.h"
#include "i2c.h"

extern I2C_HandleTypeDef hi2c1;

bool pca9685Init(void) {
    uint8_t sleep_mode = 0x10;
    if (HAL_I2C_Mem_Write(&hi2c1, PCA9685_ADDR, REG_MODE1, 1, &sleep_mode, 1, 10) != HAL_OK) return false;

    uint8_t mode2 = 0x04;
    if (HAL_I2C_Mem_Write(&hi2c1, PCA9685_ADDR, REG_MODE2, 1, &mode2, 1, 10) != HAL_OK) return false;

    uint8_t prescale = 121;
    if (HAL_I2C_Mem_Write(&hi2c1, PCA9685_ADDR, REG_PRESCALE, 1, &prescale, 1, 10) != HAL_OK) return false;

    uint8_t mode1 = 0x00;
    if (HAL_I2C_Mem_Write(&hi2c1, PCA9685_ADDR, REG_MODE1, 1, &mode1, 1, 10) != HAL_OK) return false;
    HAL_Delay(5);  // ✅ 1 → 5ms로 변경

    mode1 = 0xA1;  // ✅ AUTO_INCREMENT | ALLCALL (0xA0 → 0xA1)
    if (HAL_I2C_Mem_Write(&hi2c1, PCA9685_ADDR, REG_MODE1, 1, &mode1, 1, 10) != HAL_OK) return false;
    HAL_Delay(5);

    return true;
}

void pca9685SetAngle(uint8_t ch, float angle) {
    if (angle < 0.0f)   angle = 0.0f;
    if (angle > 180.0f) angle = 180.0f;

    uint16_t min_tick, max_tick;

    if (ch == 3) {
        // MG996R
        min_tick = 125;
        max_tick = 625;
    } else {
        // AD002 (ch 0, 1, 2)
        min_tick = 102;
        max_tick = 512;
    }

    uint16_t off_tick = (uint16_t)(min_tick + (angle / 180.0f) * (max_tick - min_tick));

    uint8_t pwm_data[4] = {
        0x00, 0x00,
        (uint8_t)(off_tick & 0xFF),
        (uint8_t)((off_tick >> 8) & 0x0F)
    };

    HAL_I2C_Mem_Write(&hi2c1, PCA9685_ADDR, REG_LED0_ON_L + (ch * 4), 1, pwm_data, 4, 100);
}