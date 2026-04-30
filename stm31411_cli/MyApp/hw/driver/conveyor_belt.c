#include "conveyor_belt.h"

extern TIM_HandleTypeDef htim2;

/* ============================================================
 * 컨베이어 고정 설정
 * ============================================================ */
#define CONVEYOR_FIXED_PULSE    1250u   /* 고정 속도 */

/* ============================================================
 * 깃발 서보 고정 펄스
 * ============================================================ */
#define FLAG_PULSE_LEFT    1100u
#define FLAG_PULSE_CENTER  1500u
#define FLAG_PULSE_RIGHT   1900u

static FlagPos_t s_flag_pos = FLAG_CENTER;

/* ============================================================
 * 컨베이어 내부 상태
 * ============================================================ */
static ConveyorState_t s_state = {
    .speed     = CONVEYOR_STOP,
    .direction = CONVEYOR_DIR_BACKWARD,
    .running   = false,
    .pulse_us  = SERVO_PULSE_CENTER,
};

/* ============================================================
 * 내부 헬퍼
 * ============================================================ */
static void apply_pulse(uint32_t pulse_us)
{
    if (pulse_us < SERVO_PULSE_MIN) pulse_us = SERVO_PULSE_MIN;
    if (pulse_us > SERVO_PULSE_MAX) pulse_us = SERVO_PULSE_MAX;
    s_state.pulse_us = pulse_us;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pulse_us);
}

static void flag_apply_pulse(uint32_t pulse_us)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, pulse_us);
}

/* ============================================================
 * 깃발 Public API
 * ============================================================ */
void Flag_Init(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    flag_apply_pulse(FLAG_PULSE_CENTER);
    s_flag_pos = FLAG_CENTER;
    cliPrintf("Flag servo init OK (center=1500)\r\n");
}

void Flag_SetLeft(void)
{
    s_flag_pos = FLAG_LEFT;
    flag_apply_pulse(FLAG_PULSE_LEFT);
    cliPrintf("Flag: LEFT (1100)\r\n");
}

void Flag_SetRight(void)
{
    s_flag_pos = FLAG_RIGHT;
    flag_apply_pulse(FLAG_PULSE_RIGHT);
    cliPrintf("Flag: RIGHT (1900)\r\n");
}

void Flag_SetCenter(void)
{
    s_flag_pos = FLAG_CENTER;
    flag_apply_pulse(FLAG_PULSE_CENTER);
    cliPrintf("Flag: CENTER (1500)\r\n");
}

FlagPos_t Flag_GetPos(void)
{
    return s_flag_pos;
}

/* ============================================================
 * CLI: flag
 *   flag left    → 1100
 *   flag right   → 1900
 *   flag center  → 1500
 *   flag status
 * ============================================================ */
void cliFlag(uint8_t argc, char **argv)
{
    if (argc < 2)
    {
        cliPrintf("Usage: flag [left|right|center|status]\r\n");
        return;
    }

    if (strcmp(argv[1], "left") == 0)
        Flag_SetLeft();
    else if (strcmp(argv[1], "right") == 0)
        Flag_SetRight();
    else if (strcmp(argv[1], "center") == 0)
        Flag_SetCenter();
    else if (strcmp(argv[1], "status") == 0)
    {
        const char *pos_str[] = {"LEFT", "CENTER", "RIGHT"};
        uint32_t   pulse[]    = {FLAG_PULSE_LEFT, FLAG_PULSE_CENTER, FLAG_PULSE_RIGHT};
        cliPrintf("Flag: %s (%lu us)\r\n", pos_str[s_flag_pos], pulse[s_flag_pos]);
    }
    else
        cliPrintf("Usage: flag [left|right|center|status]\r\n");
}

/* ============================================================
 * 컨베이어 Public API
 * ============================================================ */
void Conveyor_Init(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    apply_pulse(SERVO_PULSE_CENTER);

    s_state.speed     = CONVEYOR_STOP;
    s_state.direction = CONVEYOR_DIR_BACKWARD;
    s_state.running   = false;

    HAL_Delay(500);
    Flag_Init();
}

void Conveyor_Start(ConveyorSpeed_t speed, ConveyorDir_t direction)
{
    (void)speed;
    (void)direction;

    s_state.speed     = CONVEYOR_SLOW;
    s_state.direction = CONVEYOR_DIR_BACKWARD;
    s_state.running   = true;

    apply_pulse(CONVEYOR_FIXED_PULSE);
}

void Conveyor_Stop(void)
{
    s_state.speed   = CONVEYOR_STOP;
    s_state.running = false;
    apply_pulse(SERVO_PULSE_CENTER);
}

void Conveyor_SetSpeed(ConveyorSpeed_t speed)   { (void)speed; }
void Conveyor_ToggleDirection(void)             {}

const ConveyorState_t* Conveyor_GetState(void)
{
    return &s_state;
}

void Conveyor_SetPulse(uint32_t pulse_us)
{
    apply_pulse(pulse_us);
}