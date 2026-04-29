#ifndef _HW_DRIVER_HX711_H_
#define _HW_DRIVER_HX711_H_

/*
 * HX711.h
 * ─────────────────────────────────────────────────────────────
 *  HX711 24-bit ADC 로드셀 드라이버
 *
 *  핀 배치:
 *    SCK  → PA0 (GPIO_Output)
 *    DT   → PA1 (GPIO_Input)
 *
 *  게인 설정 (pulse 수):
 *    HX711_GAIN_128 : Channel A, Gain 128 (기본값)
 *    HX711_GAIN_32  : Channel B, Gain 32
 *    HX711_GAIN_64  : Channel A, Gain 64
 * ─────────────────────────────────────────────────────────────
 */

#include "hw_def.h"

/* ── 핀 정의 ─────────────────────────────────────────────── */
#define HX711_SCK_PORT   GPIOA
#define HX711_SCK_PIN    GPIO_PIN_0

#define HX711_DT_PORT    GPIOA
#define HX711_DT_PIN     GPIO_PIN_1

/* ── 게인 설정 ───────────────────────────────────────────── */
typedef enum
{
    HX711_GAIN_128 = 1,
    HX711_GAIN_32  = 2,
    HX711_GAIN_64  = 3,
} HX711_Gain;

/* ── 공개 API ────────────────────────────────────────────── */
bool    hx711Init(HX711_Gain gain);
bool    hx711IsReady(void);
int32_t hx711Read(void);

#endif /* _HW_DRIVER_HX711_H_ */