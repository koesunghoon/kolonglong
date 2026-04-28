#include "conveyor_belt.h"


extern TIM_HandleTypeDef htim2;

static const uint32_t speed_pulse_offset[CONVEYOR_SPEED_LEVELS] = {
    0,    /* STOP   : offset 0   → pulse = 1500 */
    100,  /* SLOW   : offset 100 → pulse = 1600 or 1400 */
    200,  /* MEDIUM : offset 200 → pulse = 1700 or 1300 */
    300,  /* FAST   : offset 300 → pulse = 1800 or 1200 */
    400,  /* TURBO  : offset 400 → pulse = 1900 or 1100 */
};

/* 내부 상태 */
static ConveyorState_t s_state = {
    .speed     = CONVEYOR_STOP,
    .direction = CONVEYOR_DIR_FORWARD,
    .running   = false,
    .pulse_us  = SERVO_PULSE_CENTER,
};

/* ============================================================
 * 내부 헬퍼: 펄스 적용
 * ============================================================ */
static void apply_pulse(uint32_t pulse_us)
{
    /* 범위 클램핑 */
    if (pulse_us < SERVO_PULSE_MIN) pulse_us = SERVO_PULSE_MIN;
    if (pulse_us > SERVO_PULSE_MAX) pulse_us = SERVO_PULSE_MAX;

    s_state.pulse_us = pulse_us;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pulse_us);
}

/* ============================================================
 * 내부 헬퍼: 속도 + 방향으로 펄스 계산
 * ============================================================ */
static uint32_t calc_pulse(ConveyorSpeed_t speed, ConveyorDir_t dir)
{
    uint32_t offset = speed_pulse_offset[speed];

    if (speed == CONVEYOR_STOP) {
        return SERVO_PULSE_CENTER;
    }

    if (dir == CONVEYOR_DIR_FORWARD) {
        return SERVO_PULSE_CENTER + offset;
    } else {
        return SERVO_PULSE_CENTER - offset;
    }
}

/* ============================================================
 * Public API 구현
 * ============================================================ */


void Conveyor_Init(void)
{
    // /* PWM 출력 시작 */
    // HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

    // /* 서보 중립(정지) 위치로 초기화 */
    // apply_pulse(SERVO_PULSE_CENTER);

    // s_state.speed     = CONVEYOR_STOP;
    // s_state.direction = CONVEYOR_DIR_FORWARD;
    // s_state.running   = false;

    // /* 서보 안정화 대기 */
    // HAL_Delay(500);
    HAL_StatusTypeDef ret = HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    cliPrintf("PWM Start ret=%d\r\n", ret);

    apply_pulse(SERVO_PULSE_CENTER);
    cliPrintf("Pulse applied: %lu\r\n", s_state.pulse_us);

    s_state.speed     = CONVEYOR_STOP;
    s_state.direction = CONVEYOR_DIR_FORWARD;
    s_state.running   = false;

    HAL_Delay(500);
}


void Conveyor_Start(ConveyorSpeed_t speed, ConveyorDir_t direction)
{
    if (speed == CONVEYOR_STOP) {
        Conveyor_Stop();
        return;
    }

    s_state.speed     = speed;
    s_state.direction = direction;
    s_state.running   = true;

    apply_pulse(calc_pulse(speed, direction));
}

void Conveyor_Stop(void)
{
    s_state.speed   = CONVEYOR_STOP;
    s_state.running = false;

    apply_pulse(SERVO_PULSE_CENTER);
}

void Conveyor_SetSpeed(ConveyorSpeed_t speed)
{
    if (speed == CONVEYOR_STOP) {
        Conveyor_Stop();
        return;
    }

    s_state.speed   = speed;
    s_state.running = true;

    apply_pulse(calc_pulse(speed, s_state.direction));
}

void Conveyor_ToggleDirection(void)
{
    if (s_state.direction == CONVEYOR_DIR_FORWARD) {
        s_state.direction = CONVEYOR_DIR_BACKWARD;
    } else {
        s_state.direction = CONVEYOR_DIR_FORWARD;
    }

    /* 실행 중이면 즉시 방향 반영 */
    if (s_state.running) {
        apply_pulse(calc_pulse(s_state.speed, s_state.direction));
    }
}

const ConveyorState_t* Conveyor_GetState(void)
{
    return &s_state;
}

void Conveyor_SetPulse(uint32_t pulse_us)
{
    apply_pulse(pulse_us);
}