#ifndef CONVEYOR_BELT_H
#define CONVEYOR_BELT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "ap.h"


/* ── PWM 타이머 핸들 (main.c에서 extern으로 연결) ── */
//extern TIM_HandleTypeDef htim2;

/* ── 서보 펄스 범위 (단위: us = TIM tick) ── */
#define SERVO_PULSE_MIN        500u   /* 0.5 ms  → 약 0°   */
#define SERVO_PULSE_CENTER     1500u  /* 1.5 ms  → 약 90°  */
#define SERVO_PULSE_MAX        2500u  /* 2.5 ms  → 약 180° */

/* ── 컨베이어 벨트 속도 레벨 (0 ~ 5) ── */
typedef enum {
    CONVEYOR_STOP   = 0,
    CONVEYOR_SLOW   = 1,
    CONVEYOR_MEDIUM = 2,
    CONVEYOR_FAST   = 3,
    CONVEYOR_TURBO  = 4,
    CONVEYOR_SPEED_LEVELS
} ConveyorSpeed_t;

/* ── 컨베이어 방향 ── */
typedef enum {
    CONVEYOR_DIR_FORWARD  = 0,
    CONVEYOR_DIR_BACKWARD = 1
} ConveyorDir_t;

/* ── 컨베이어 상태 구조체 ── */
typedef struct {
    ConveyorSpeed_t speed;
    ConveyorDir_t   direction;
    bool            running;
    uint32_t        pulse_us;   /* 현재 PWM 펄스 폭 (us) */
} ConveyorState_t;

/* ============================================================
 * Public API
 * ============================================================ */

/**
 * @brief  컨베이어 벨트 초기화 (PWM 시작 + 서보 중립 위치)
 */
void Conveyor_Init(void);

/**
 * @brief  컨베이어 시작
 * @param  speed    : CONVEYOR_SLOW ~ CONVEYOR_TURBO
 * @param  direction: CONVEYOR_DIR_FORWARD / BACKWARD
 */
void Conveyor_Start(ConveyorSpeed_t speed, ConveyorDir_t direction);

/**
 * @brief  컨베이어 정지 (서보 중립)
 */
void Conveyor_Stop(void);

/**
 * @brief  속도 변경 (방향 유지)
 * @param  speed : 새 속도 레벨
 */
void Conveyor_SetSpeed(ConveyorSpeed_t speed);

/**
 * @brief  방향 전환 (속도 유지)
 */
void Conveyor_ToggleDirection(void);

/**
 * @brief  현재 상태 반환
 * @return ConveyorState_t 포인터 (read-only)
 */
const ConveyorState_t* Conveyor_GetState(void);

/**
 * @brief  직접 PWM 펄스 설정 (us 단위, 500~2500)
 * @param  pulse_us : 펄스 폭
 */
void Conveyor_SetPulse(uint32_t pulse_us);

#ifdef __cplusplus
}
#endif
#endif /* CONVEYOR_BELT_H */