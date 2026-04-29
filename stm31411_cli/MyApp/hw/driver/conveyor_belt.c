#include "conveyor_belt.h"

extern TIM_HandleTypeDef htim2;

/* ============================================================
 * 고정 설정값
 * ============================================================ */
#define CONVEYOR_FIXED_OFFSET   50u   /* SLOW(100)보다 절반 → 1500-50 = 1450us */
#define CONVEYOR_FIXED_PULSE    (SERVO_PULSE_CENTER - CONVEYOR_FIXED_OFFSET)

/* 내부 상태 */
static ConveyorState_t s_state = {
    .speed     = CONVEYOR_STOP,
    .direction = CONVEYOR_DIR_BACKWARD,   /* 방향 고정 */
    .running   = false,
    .pulse_us  = SERVO_PULSE_CENTER,
};

/* ============================================================
 * 내부 헬퍼: 펄스 적용
 * ============================================================ */
static void apply_pulse(uint32_t pulse_us)
{
    if (pulse_us < SERVO_PULSE_MIN) pulse_us = SERVO_PULSE_MIN;
    if (pulse_us > SERVO_PULSE_MAX) pulse_us = SERVO_PULSE_MAX;

    s_state.pulse_us = pulse_us;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pulse_us);
}

/* ============================================================
 * Public API 구현
 * ============================================================ */

void Conveyor_Init(void)
{
    HAL_StatusTypeDef ret = HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    cliPrintf("PWM Start ret=%d\r\n", ret);

    apply_pulse(SERVO_PULSE_CENTER);
    cliPrintf("Pulse applied: %lu\r\n", s_state.pulse_us);

    s_state.speed     = CONVEYOR_STOP;
    s_state.direction = CONVEYOR_DIR_BACKWARD;
    s_state.running   = false;

    HAL_Delay(500);
}

void Conveyor_Start(ConveyorSpeed_t speed, ConveyorDir_t direction)
{
    /* 인자 무시 → 고정 속도/방향으로만 동작 */
    (void)speed;
    (void)direction;

    s_state.speed     = CONVEYOR_SLOW;
    s_state.direction = CONVEYOR_DIR_BACKWARD;
    s_state.running   = true;

    apply_pulse(CONVEYOR_FIXED_PULSE);
    cliPrintf("Conveyor: FIXED BACKWARD pulse=%lu\r\n", CONVEYOR_FIXED_PULSE);
}

void Conveyor_Stop(void)
{
    s_state.speed   = CONVEYOR_STOP;
    s_state.running = false;

    apply_pulse(SERVO_PULSE_CENTER);
}

/* 아래 함수들은 고정 모드에서 동작 없음 (호환성 유지) */
void Conveyor_SetSpeed(ConveyorSpeed_t speed)
{
    (void)speed;
    /* 고정 속도 모드 → 무시 */
    cliPrintf("Conveyor: fixed speed mode, SetSpeed ignored\r\n");
}

void Conveyor_ToggleDirection(void)
{
    /* 방향 고정 → 무시 */
    cliPrintf("Conveyor: fixed BACKWARD mode, ToggleDirection ignored\r\n");
}

const ConveyorState_t* Conveyor_GetState(void)
{
    return &s_state;
}

void Conveyor_SetPulse(uint32_t pulse_us)
{
    apply_pulse(pulse_us);
}