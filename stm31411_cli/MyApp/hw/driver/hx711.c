#include "hx711.h"
#include "FreeRTOS.h"
#include "task.h"

/*
 * HX711.c
 * ─────────────────────────────────────────────────────────────
 *  DWT 기반 us 딜레이 + Critical Section으로 24비트 읽기
 *
 *  [DWT delay 원리]
 *    CoreDebug->DEMCR으로 DWT 활성화
 *    DWT->CYCCNT 레지스터가 CPU 클럭마다 1씩 증가
 *    84MHz 기준: 1us = 84 사이클
 *
 *  [HX711 타이밍]
 *    PD_SCK HIGH/LOW 최소 0.2us → 1us로 여유있게
 * ─────────────────────────────────────────────────────────────
 */

/* ── DWT us 딜레이 ───────────────────────────────────────── */
static bool s_dwt_initialized = false;

static void dwt_init(void)
{
    if (s_dwt_initialized) return;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT       = 0U;
    DWT->CTRL        |= DWT_CTRL_CYCCNTENA_Msk;
    s_dwt_initialized = true;
}

static void delay_us(uint32_t us)
{
    uint32_t target = DWT->CYCCNT + (us * (SystemCoreClock / 1000000U));
    while ((int32_t)(target - DWT->CYCCNT) > 0) {}
}

/* ── 내부 상태 ───────────────────────────────────────────── */
static HX711_Gain s_gain = HX711_GAIN_128;

/* ── 핀 헬퍼 ─────────────────────────────────────────────── */
static inline void sck_high(void)
{
    HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_SET);
}

static inline void sck_low(void)
{
    HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_RESET);
}

static inline GPIO_PinState dt_read(void)
{
    return HAL_GPIO_ReadPin(HX711_DT_PORT, HX711_DT_PIN);
}

/* ── 초기화 ─────────────────────────────────────────────── */
bool hx711Init(HX711_Gain gain)
{
    dwt_init();
    s_gain = gain;
    sck_low();
    return true;
}

/* ── 데이터 준비 확인 ────────────────────────────────────── */
bool hx711IsReady(void)
{
    return (dt_read() == GPIO_PIN_RESET);
}

/* ── 24비트 읽기 ─────────────────────────────────────────── */
int32_t hx711Read(void)
{
    uint32_t raw    = 0U;
    int32_t  result = 0;
    uint8_t  i;

    taskENTER_CRITICAL();

    for (i = 0U; i < 24U; i++)
    {
        sck_high();
        delay_us(1U);
        raw <<= 1U;
        if (dt_read() == GPIO_PIN_SET) raw |= 0x01U;
        sck_low();
        delay_us(1U);
    }

    /* 게인 설정 펄스 */
    for (i = 0U; i < (uint8_t)s_gain; i++)
    {
        sck_high();
        delay_us(1U);
        sck_low();
        delay_us(1U);
    }

    taskEXIT_CRITICAL();

    /* 2의 보수 변환 (24비트 → int32_t) */
    if (raw & 0x800000U)
        result = (int32_t)(raw | 0xFF000000U);
    else
        result = (int32_t)raw;

    return result;
}