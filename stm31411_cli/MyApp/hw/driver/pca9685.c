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
    HAL_Delay(5);

    mode1 = 0xA1;
    if (HAL_I2C_Mem_Write(&hi2c1, PCA9685_ADDR, REG_MODE1, 1, &mode1, 1, 10) != HAL_OK) return false;
    HAL_Delay(5);

    return true;
}

void pca9685SetAngle(uint8_t ch, float angle) {
    if (angle < 0.0f)   angle = 0.0f;
    if (angle > 180.0f) angle = 180.0f;

    uint16_t min_tick, max_tick;

    if (ch == 3) {
        // MG996R - 80도가 실제 90도
        min_tick = 150;
        max_tick = 560;
    } else if (ch == 4) {
        // MG996R - 50도가 실제 90도
        min_tick = 150;
        max_tick = 480;
    } else {
        // AD002 (ch 0, 1, 2)
        min_tick = 115;
        max_tick = 490;
    }

    uint16_t off_tick = (uint16_t)(min_tick + (angle / 180.0f) * (max_tick - min_tick));

    uint8_t pwm_data[4] = {
        0x00, 0x00,
        (uint8_t)(off_tick & 0xFF),
        (uint8_t)((off_tick >> 8) & 0x0F)
    };

    HAL_I2C_Mem_Write(&hi2c1, PCA9685_ADDR, REG_LED0_ON_L + (ch * 4), 1, pwm_data, 4, 100);
}

void pca9685SetAngleSmooth(uint8_t ch, float target_angle, uint16_t duration_ms) {
    // 현재 각도를 저장하는 배열
    static float current_angle[16] = {
    90.0f,  // ch 0
    90.0f,  // ch 1
    90.0f,  // ch 2
    90.0f,  // ch 3
    90.0f,  // ch 4
    90.0f,  // ch 5
    90.0f,  // ch 6
    90.0f,  // ch 7
    90.0f,  // ch 8
    90.0f,  // ch 9
    90.0f,  // ch 10
    90.0f,  // ch 11
    90.0f,  // ch 12
    90.0f,  // ch 13
    90.0f,  // ch 14
    90.0f   // ch 15
};

    float start = current_angle[ch];
    float diff  = target_angle - start;
    uint16_t steps = duration_ms / 10; // 10ms마다 한 스텝

    if (steps == 0) steps = 1;

    for (uint16_t i = 1; i <= steps; i++) {
        float angle = start + diff * ((float)i / steps);
        pca9685SetAngle(ch, angle);
        osDelay(10);
    }

    current_angle[ch] = target_angle;
}

// 두 집게 서보모터 동시 동작 함수
void pca9685SetAngleSmoothDual(uint8_t ch0, float target0, uint8_t ch1, float target1, uint16_t duration_ms) {
    static float current_angle[16] = {
        90.0f, 90.0f, 90.0f, 90.0f, 90.0f, 90.0f, 90.0f, 90.0f,
        90.0f, 90.0f, 90.0f, 90.0f, 90.0f, 90.0f, 90.0f, 90.0f
    };

    float start0 = current_angle[ch0];
    float start1 = current_angle[ch1];
    float diff0  = target0 - start0;
    float diff1  = target1 - start1;

    uint16_t steps = duration_ms / 10;
    if (steps == 0) steps = 1;

    for (uint16_t i = 1; i <= steps; i++) {
        float angle0 = start0 + diff0 * ((float)i / steps);
        float angle1 = start1 + diff1 * ((float)i / steps);
        pca9685SetAngle(ch0, angle0);  // ch0 write
        pca9685SetAngle(ch1, angle1);  // ch1 write (I2C라 수십us 차이, 육안으론 동시)
        osDelay(10);
    }

    current_angle[ch0] = target0;
    current_angle[ch1] = target1;
}