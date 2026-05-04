#ifndef __SYSTEM_EVENTS_H_
#define __SYSTEM_EVENTS_H_

/*
 * system_events.h
 * ─────────────────────────────────────────────────────────────
 *  FreeRTOS EventGroup 비트 정의
 *
 *  [Task 간 통신 구조]
 *
 *  LoadCell Task
 *      무게 확정 → EVT_WEIGHT_OK 설정
 *          ↓
 *  Trigger Task
 *      EVT_WEIGHT_OK 대기 → Flag 설정 → 딜레이 → Flag CENTER 복귀
 *
 *  RobotArm Task (나중에 통합 시 #if 0 제거)
 *      EVT_WEIGHT_OK 대기 → 집기 동작 → EVT_ARM_DONE 설정
 * ─────────────────────────────────────────────────────────────
 */

#include "cmsis_os2.h"

/* ── EventGroup 비트 정의 ────────────────────────────────── */
#define EVT_WEIGHT_OK       (1U << 0)   /* 무게 확정 */
#define EVT_ARM_DONE        (1U << 1)   /* 로봇팔 동작 완료 */
#define EVT_POSITION_OK     (1U << 2)   /* 물건 위치 도착 */

/* ── EventGroup 핸들 (ap.c에서 생성) ────────────────────── */
extern osEventFlagsId_t g_sys_event;

#endif /* __SYSTEM_EVENTS_H_ */