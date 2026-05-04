#include "ap.h"
#include <math.h>
#include "pca9685.h"

/* ═══════════════════════════════════════════════════════════
 *  LoadCell Task
 * ═══════════════════════════════════════════════════════════ */

/* ── LoadCell 설정 ───────────────────────────────────────── */
#define LOADCELL_TASK_PERIOD_MS    250
#define TARE_SAMPLE_COUNT          20
#define HX711_READY_TIMEOUT_MS     500

/* 하드코딩 기준값 — loadcell rawtare 로 확인 후 교체 */
#define DEFAULT_TARE_OFFSET        (-5757)

/* 캘리브레이션 계수 */
#define SCALE_FACTOR_DEFAULT       50.46f

/* 데드밴드 */
#define DEADBAND_GRAM              2.0f

/* 중앙값 필터 윈도우 */
#define MEDIAN_SIZE                7

/* 적응형 EMA */
#define EMA_ALPHA_SLOW             0.1f
#define EMA_ALPHA_FAST             0.6f
#define EMA_CHANGE_THRESH_G        5.0f

/* 안정화 감지 */
#define STABLE_COUNT               5
#define STABLE_THRESH_G            2.0f

/* 히스테리시스 — 물체 감지/해제 기준 */
#define OBJECT_DETECT_G            25.0f   /* 이 값 이상이면 물체 올라왔다고 판단 */
#define OBJECT_RELEASE_G           12.0f   /* 이 값 이하로 떨어지면 물체 제거됐다고 판단 */

/* Zero tracking */
#define ZERO_TRACK_LIMIT_G         15.0f   /* 이 값 이하일 때만 zero tracking 동작 */
#define ZERO_TRACK_STABLE_G         1.0f   /* 직전 무게와 변화량이 이 이하일 때만 동작 */
#define ZERO_TRACK_ALPHA            0.02f  /* offset 따라가는 속도 (작을수록 느리게) */
#define ZERO_TRACK_RELEASE_WAIT_MS  3000   /* 물체 해제 후 대기 시간 (ms) */

/* ── 전역 상태 ───────────────────────────────────────────── */
static volatile int32_t  g_tare_offset         = DEFAULT_TARE_OFFSET;
static volatile float    g_scale_factor        = SCALE_FACTOR_DEFAULT;
static volatile bool     g_tare_request        = false;
static volatile bool     g_loadcell_auto_print = false;
static volatile bool     g_invert              = true;

/* ── 중앙값 필터 ─────────────────────────────────────────── */
static int32_t g_med_buf[MEDIAN_SIZE] = {0};
static uint8_t g_med_idx  = 0;
static bool    g_med_full = false;

static int32_t medianFilter(int32_t new_raw)
{
    g_med_buf[g_med_idx] = new_raw;
    g_med_idx = (g_med_idx + 1) % MEDIAN_SIZE;
    if (g_med_idx == 0) g_med_full = true;

    uint8_t count = g_med_full ? MEDIAN_SIZE : g_med_idx;
    int32_t sorted[MEDIAN_SIZE];
    for (uint8_t i = 0; i < count; i++) sorted[i] = g_med_buf[i];

    for (uint8_t i = 0; i < count - 1; i++)
        for (uint8_t j = 0; j < count - i - 1; j++)
            if (sorted[j] > sorted[j+1])
            {
                int32_t tmp  = sorted[j];
                sorted[j]    = sorted[j+1];
                sorted[j+1]  = tmp;
            }

    return sorted[count / 2];
}

/* ── 적응형 EMA ──────────────────────────────────────────── */
static float g_ema_value       = 0.0f;
static bool  g_ema_initialized = false;

static float adaptiveEma(int32_t new_raw)
{
    if (!g_ema_initialized)
    {
        g_ema_value       = (float)new_raw;
        g_ema_initialized = true;
        return g_ema_value;
    }

    float diff_g = ((float)new_raw - g_ema_value) / g_scale_factor;
    if (g_invert) diff_g = -diff_g;

    float alpha = ((diff_g > EMA_CHANGE_THRESH_G) || (diff_g < -EMA_CHANGE_THRESH_G))
                  ? EMA_ALPHA_FAST : EMA_ALPHA_SLOW;

    g_ema_value = alpha * (float)new_raw + (1.0f - alpha) * g_ema_value;
    return g_ema_value;
}

/* ── 안정화 감지 ─────────────────────────────────────────── */
volatile float   g_stable_last   = 0.0f;
volatile uint8_t g_stable_count  = 0;
volatile float   g_stable_weight = 0.0f;

static void updateStable(float weight)
{
    float diff = weight - g_stable_last;
    if (diff < 0.0f) diff = -diff;

    if (diff <= STABLE_THRESH_G)
    {
        g_stable_count++;
        if (g_stable_count >= STABLE_COUNT)
        {
            g_stable_weight = weight;
            g_stable_count  = STABLE_COUNT;
        }
    }
    else
    {
        g_stable_count  = 0;
        g_stable_weight = weight;
    }
    g_stable_last = weight;
}

/* ── 필터 리셋 ───────────────────────────────────────────── */
static void resetFilters(int32_t tare_val)
{
    for (uint8_t i = 0; i < MEDIAN_SIZE; i++) g_med_buf[i] = tare_val;
    g_med_idx  = 0;
    g_med_full = true;

    g_ema_value       = (float)tare_val;
    g_ema_initialized = true;

    g_stable_last   = 0.0f;
    g_stable_count  = 0;
    g_stable_weight = 0.0f;
}

/* ── raw → 그램 변환 ─────────────────────────────────────── */
static float calcWeight(int32_t raw)
{
    int32_t median   = medianFilter(raw);
    float   filtered = adaptiveEma(median);
    int32_t net      = (int32_t)filtered - g_tare_offset;
    if (g_invert) net = -net;
    float   weight   = (float)net / g_scale_factor;
    if (weight < DEADBAND_GRAM) weight = 0.0f;
    return weight;
}

/* ── HX711 읽기 헬퍼 ─────────────────────────────────────── */
static bool readRaw(int32_t *out)
{
    uint32_t timeout = HX711_READY_TIMEOUT_MS;
    while (!hx711IsReady() && timeout > 0)
    {
        osDelay(1);
        timeout--;
    }
    if (timeout == 0) return false;
    *out = hx711Read();
    return true;
}

/* ── Tare 실행 ───────────────────────────────────────────── */
static void runTare(void)
{
    int32_t sum = 0;
    int32_t raw;

    cliPrintf("Tare 시작... (%d회 평균)\r\n", TARE_SAMPLE_COUNT);

    for (uint8_t i = 0; i < TARE_SAMPLE_COUNT; i++)
    {
        if (!readRaw(&raw))
        {
            cliPrintf("HX711 응답 없음 (Tare 중단)\r\n");
            return;
        }
        sum += raw;
        osDelay(10);
    }

    g_tare_offset = sum / TARE_SAMPLE_COUNT;
    resetFilters(g_tare_offset);
    cliPrintf("Tare 완료. offset = %ld\r\n", (long)g_tare_offset);
}

/* ── CLI: loadcell ───────────────────────────────────────── */
void cliLoadCell(uint8_t argc, char **argv)
{
    if (argc < 2)
    {
        cliPrintf("Usage: loadcell tare\r\n");
        cliPrintf("       loadcell rawtare\r\n");
        cliPrintf("       loadcell read\r\n");
        cliPrintf("       loadcell auto [on|off]\r\n");
        cliPrintf("       loadcell scale [value]\r\n");
        cliPrintf("       loadcell invert\r\n");
        return;
    }

    if (strcmp(argv[1], "tare") == 0)
    {
        g_tare_request = true;
        cliPrintf("Tare 요청됨.\r\n");
    }
    else if (strcmp(argv[1], "rawtare") == 0)
    {
        int32_t sum = 0;
        int32_t raw;
        for (uint8_t i = 0; i < 20; i++)
        {
            if (readRaw(&raw)) sum += raw;
            osDelay(10);
        }
        cliPrintf("현재 raw 평균: %ld\r\n", (long)(sum / 20));
        cliPrintf("-> ap.c의 DEFAULT_TARE_OFFSET을 이 값으로 교체하세요.\r\n");
    }
    else if (strcmp(argv[1], "read") == 0)
    {
        int32_t raw;
        if (!readRaw(&raw)) { cliPrintf("HX711 응답 없음\r\n"); return; }
        cliPrintf("raw=%ld  weight=%.2f g\r\n", (long)raw, calcWeight(raw));
    }
    else if (strcmp(argv[1], "auto") == 0 && argc == 3)
    {
        if (strcmp(argv[2], "on") == 0)
        {
            g_loadcell_auto_print = true;
            cliPrintf("LoadCell 자동 출력 ON\r\n");
        }
        else if (strcmp(argv[2], "off") == 0)
        {
            g_loadcell_auto_print = false;
            cliPrintf("LoadCell 자동 출력 OFF\r\n");
        }
        else { cliPrintf("Usage: loadcell auto [on|off]\r\n"); }
    }
    else if (strcmp(argv[1], "invert") == 0)
    {
        g_invert = !g_invert;
        cliPrintf("부호 반전 %s\r\n", g_invert ? "ON" : "OFF");
    }
    else if (strcmp(argv[1], "scale") == 0 && argc == 3)
    {
        float val = strtof(argv[2], NULL);
        if (val > 0.0f) { g_scale_factor = val; cliPrintf("Scale factor = %.2f\r\n", g_scale_factor); }
        else              cliPrintf("Invalid scale factor\r\n");
    }
    else { cliPrintf("Unknown command\r\n"); }
}

/* ── FreeRTOS Task: loadCellSystemTask ──────────────────── */
/* ── 분류 기준 ───────────────────────────────────────────── */
#define CLASSIFY_HEAVY_G           60.0f   /* 이 값 이상 → 무거운 물체 (FLAG LEFT) */

/* ── 컨베이어 동작 시간 ──────────────────────────────────── */
#define CONVEYOR_RUN_MS            3000    /* 물체가 플래그 통과할 시간 — 수정 가능 */

/* ── 시스템 상태머신 ─────────────────────────────────────── */
typedef enum {
    STATE_WAIT_OBJECT,      /* 로드셀에 물건 올라오길 대기 */
    STATE_MEASURING,        /* 무게 측정 중 (stable 확정 대기) */
    STATE_ARM_MOVING,       /* 로봇팔 동작 중 */
    STATE_CONVEYOR_RUNNING, /* 컨베이어 동작 중 */
    STATE_DONE,             /* 분류 완료, 초기화 후 대기로 복귀 */
    STATE_IDLE,             /* 시스템 정지 (system off) */
} SystemState_t;

static volatile SystemState_t g_system_state   = STATE_IDLE;
static volatile bool          g_system_enabled = false;  /* system on/off */

/* ── 세마포어: 로봇팔 완료 트리거 ───────────────────────── */
osSemaphoreId_t g_arm_done_sem = NULL;

/* ── arm_running 플래그 ──────────────────────────────────── */
volatile bool arm_running = false;

/* ── 측정 결과 공유 변수 (systemTask가 읽음) ─────────────── */
volatile bool  g_object_present = false;   /* 히스테리시스 상태 */

/* ================================================================
 *  loadCellSystemTask  —  측정 전용
 *  raw → 필터 → stable → 히스테리시스 → zero tracking
 * ================================================================ */
void loadCellSystemTask(void *argument)
{
    int32_t  raw             = 0;
    uint32_t release_time_ms = 0;
    float    prev_weight_g   = 0.0f;

    hx711Init(HX711_GAIN_128);
    LOG_INF("HX711 Init OK");

    osDelay(500);
    runTare();

    for (;;)
    {
        /* ── 수동 tare 요청 처리 ──────────────────────────── */
        if (g_tare_request)
        {
            g_tare_request   = false;
            runTare();
            g_object_present = false;
            release_time_ms  = 0;
            prev_weight_g    = 0.0f;
        }

        /* ── raw 읽기 ─────────────────────────────────────── */
        if (!readRaw(&raw))
        {
            LOG_WRN("HX711 timeout");
            osDelay(LOADCELL_TASK_PERIOD_MS);
            continue;
        }

        /* ── 필터 파이프라인 ──────────────────────────────── */
        float weight = calcWeight(raw);
        updateStable(weight);

        /* ── 히스테리시스: 물체 감지/해제 ───────────────── */
        if (!g_object_present)
        {
            if (g_stable_weight >= OBJECT_DETECT_G)
            {
                g_object_present = true;
                release_time_ms  = 0;
                cliPrintf("[LC] 물체 감지: %.2f g\r\n", g_stable_weight);
            }
        }
        else
        {
            if (g_stable_weight <= OBJECT_RELEASE_G)
            {
                g_object_present = false;
                release_time_ms  = osKernelGetTickCount();
                cliPrintf("[LC] 물체 제거 감지\r\n");
            }
        }

        /* ── Zero Tracking ────────────────────────────────── */
        if (!g_object_present && release_time_ms != 0)
        {
            uint32_t elapsed = osKernelGetTickCount() - release_time_ms;

            if (elapsed >= ZERO_TRACK_RELEASE_WAIT_MS &&
                fabsf(g_stable_weight)                 < ZERO_TRACK_LIMIT_G &&
                fabsf(g_stable_weight - prev_weight_g) < ZERO_TRACK_STABLE_G)
            {
                int32_t filtered_raw = (int32_t)g_ema_value;
                g_tare_offset = (int32_t)(
                    (1.0f - ZERO_TRACK_ALPHA) * (float)g_tare_offset +
                    ZERO_TRACK_ALPHA          * (float)filtered_raw
                );
            }
        }

        /* ── 자동 출력 ────────────────────────────────────── */
        if (g_loadcell_auto_print || g_system_enabled)
            cliPrintf("[LoadCell] raw=%ld  weight=%.2f g\r\n",
                      (long)raw, g_stable_weight);

        prev_weight_g = g_stable_weight;
        osDelay(LOADCELL_TASK_PERIOD_MS);
    }
}

/* ================================================================
 *  systemTask  —  상태머신 전용 (분류 흐름 조율)
 * ================================================================ */
void systemTask(void *argument)
{
    float    measured_weight   = 0.0f;
    uint32_t conveyor_start_ms = 0;

    /* loadCellSystemTask 초기화 대기 */
    osDelay(1000);

    g_system_state = STATE_IDLE;
    cliPrintf("[SYS] systemTask started — 'system on' 으로 시작\r\n");

    for (;;)
    {
        /* system off 상태면 아무것도 안 함 */
        if (!g_system_enabled)
        {
            g_system_state = STATE_IDLE;
            osDelay(100);
            continue;
        }

        switch (g_system_state)
        {
            /* system on 직후 진입점 */
            case STATE_IDLE:
                g_system_state = STATE_WAIT_OBJECT;
                cliPrintf("[SYS] -> WAIT_OBJECT\r\n");
                break;
            /* ── 물체 올라오길 대기 ───────────────────────── */
            case STATE_WAIT_OBJECT:
                if (g_object_present)
                {
                    g_stable_count = 0;   /* 측정 시작 전 카운트 리셋 */
                    cliPrintf("[SYS] -> MEASURING\r\n");
                    g_system_state = STATE_MEASURING;
                }
                break;

            /* ── stable 확정 대기 ─────────────────────────── */
            case STATE_MEASURING:
                if (!g_object_present)
                {
                    cliPrintf("[SYS] 측정 중 물체 제거 -> WAIT\r\n");
                    g_system_state = STATE_WAIT_OBJECT;
                    break;
                }
                if (g_stable_count >= STABLE_COUNT)
                {
                    measured_weight = g_stable_weight;

                    /* 플래그 방향 결정 */
                    if (measured_weight >= CLASSIFY_HEAVY_G)
                    {
                        Flag_SetLeft();
                        cliPrintf("[SYS] 무거운 물체 %.2f g -> FLAG LEFT\r\n", measured_weight);
                    }
                    else
                    {
                        Flag_SetRight();
                        cliPrintf("[SYS] 가벼운 물체 %.2f g -> FLAG RIGHT\r\n", measured_weight);
                    }

                    /* 로봇팔 시작 트리거 */
                    arm_running    = true;
                    g_system_state = STATE_ARM_MOVING;
                    cliPrintf("[SYS] -> ARM_MOVING\r\n");
                }
                break;

            /* ── 로봇팔 완료 대기 — 세마포어 수신 ───────── */
            case STATE_ARM_MOVING:
                /* 블로킹 대기 — 팔 완료될 때까지 이 태스크 sleep */
                osSemaphoreAcquire(g_arm_done_sem, osWaitForever);
                Conveyor_Start(CONVEYOR_SLOW, CONVEYOR_DIR_BACKWARD);
                conveyor_start_ms = osKernelGetTickCount();
                g_system_state    = STATE_CONVEYOR_RUNNING;
                cliPrintf("[SYS] 팔 완료 -> CONVEYOR_RUNNING\r\n");
                break;

            /* ── 컨베이어 동작 중 — 딜레이 후 정지 ─────── */
            case STATE_CONVEYOR_RUNNING:
            {
                uint32_t elapsed = osKernelGetTickCount() - conveyor_start_ms;
                if (elapsed >= CONVEYOR_RUN_MS)
                {
                    Conveyor_Stop();
                    Flag_SetCenter();
                    g_system_state = STATE_DONE;
                    cliPrintf("[SYS] 컨베이어 정지 -> DONE\r\n");
                }
                break;
            }

            /* ── 분류 완료 — 초기화 후 대기로 복귀 ─────── */
            case STATE_DONE:
                measured_weight   = 0.0f;
                conveyor_start_ms = 0;
                g_system_state    = STATE_WAIT_OBJECT;
                cliPrintf("[SYS] -> WAIT_OBJECT\r\n");
                break;

            default:
                g_system_state = STATE_WAIT_OBJECT;
                break;
        }

        osDelay(50);
    }
}

void robotArmTask(void) {

// // 1. 집게 열고
    pca9685SetAngleSmoothDual(0, 0, 1, 0, 1500);
    if (!arm_running) return;

    // 2. 고개 초기화
    pca9685SetAngleSmooth(2, 90 , 1500);
    if (!arm_running) return;
    
    // 3. 팔 내리고
    pca9685SetAngleSmooth(3, 170, 1500);
    if (!arm_running) return;

    // 4. 어깨 내리고
    pca9685SetAngleSmooth(4, 25, 1500);
    if (!arm_running) return;

    // 5. 집게 닫고
    pca9685SetAngleSmoothDual(0, 120, 1, 120, 1500);
    if (!arm_running) return;

    // 6. 어깨 올리고
    pca9685SetAngleSmooth(4, 80, 1500);
    if (!arm_running) return;

    // 7. 허리 돌리고
    pca9685SetAngleSmooth(5, 90, 1500);
    if (!arm_running) return;

    // 8. 어깨 내리고
    pca9685SetAngleSmooth(4, 55, 1500);
    if (!arm_running) return;

    // 9. 집게 열고
    pca9685SetAngleSmoothDual(0, 0, 1, 0, 1500);

    // 10. 허리 원위치
    pca9685SetAngleSmooth(5, 0, 1500);
}

void cliArm(uint8_t argc, char **argv) {
    if (argc == 2) {
        if (strcmp(argv[1], "start") == 0) {
            arm_running = true;
            cliPrintf("Arm Started\r\n");
        } else if (strcmp(argv[1], "stop") == 0) {
            arm_running = false;
            cliPrintf("Arm Stopped\r\n");
        } else {
            cliPrintf("Usage: arm [start/stop]\r\n");
        }
    } else {
        cliPrintf("Usage: arm [start/stop]\r\n");
    }
}

// button on/off  => enable/disable
void cliButton(uint8_t argc, char **argv)
{
  if (argc == 2)
  {
    if (strcmp(argv[1], "on") == 0)
    {
      buttonEnable(true);
      cliPrintf("Button Interrupt Report: ON\r\n");
    }
    else if (strcmp(argv[1], "off") == 0)
    {
      buttonEnable(false);
      cliPrintf("Button Interrupt Report: OFF\r\n");
    }
  }
  else
  {
    cliPrintf("Usage: button [on/off]\r\n");
    cliPrintf("Current Status: %s\r\n", buttonGetEnable() ? "ON" : "OFF");
  }
}

static bool isSafeAddress(uint32_t addr)
{
  // 1. f411 flash
  if (0x08000000 <= addr && addr <= 0x0807FFFF) return true;
  // 2. f411 ram
  if (0x20000000 <= addr && addr <= 0x20001FFF) return true;
  // 3. system memory
  if (0x1FFF0000 <= addr && addr <= 0x1FFF7A1F) return true;
  // 4. Peripheral register
  if (0x40000000 <= addr && addr <= 0x5FFFFFFF) return true;
  return false;
}

// md 0x8000-0000 32
void cliMd(uint8_t argc, char **argv)
{
  if (argc >= 2)
  {
    uint32_t addr = strtoul(argv[1], NULL, 16);
    uint32_t length = 16;
    if (argc >= 3) length = strtoul(argv[2], NULL, 0);

    for (uint32_t i = 0; i < length; i += 16)
    {
      cliPrintf("0x%08x : ", addr + i);
      for (uint32_t j = 0; j < 16; j++)
      {
        if (i + j < length)
        {
          uint32_t target_addr = (addr + i + j);
          if (isSafeAddress(target_addr))
          {
            uint8_t val = *((volatile uint8_t *)target_addr);
            cliPrintf("%02X ", val);
          }
          else { cliPrintf("Not valid address!!\r\n"); break; }
        }
        else { cliPrintf("   "); }
      }
      cliPrintf(" | ");
      for (uint32_t j = 0; j < 16; j++)
      {
        if (i + j < length)
        {
          uint32_t target_addr = (addr + i + j);
          if (isSafeAddress(target_addr))
          {
            uint8_t val = *((volatile uint8_t *)target_addr);
            if (val >= 0x20 && val <= 0x7E) cliPrintf("%c", val);
            else                             cliPrintf(".");
          }
          else { cliPrintf("Not valid address!!\r\n"); break; }
        }
      }
      cliPrintf("\r\n");
    }
  }
  else
  {
    cliPrintf("Usage : md [add(hex)] [length]\r\n");
    cliPrintf("        md 08000000 32\r\n");
  }
}

// argv[1] : "read" "write"
// argv[2] : pin "A5", "B12"
void cliGpio(uint8_t argc, char **argv)
{
  if (argc >= 3)
  {
    char port_char = tolower(argv[2][0]);
    int pin_num = atoi(&argv[2][1]);
    uint8_t port_idx = port_char - 'a';

    if (strcmp(argv[1], "read") == 0)
    {
      int8_t state = gpioExtRead(port_idx, pin_num);
      if (state < 0) cliPrintf("Invalid Port or Pin (ex:a5, b12)\r\n");
      else           cliPrintf("GPIO %c%d=%d\r\n", toupper(port_char), pin_num, state);
    }
    else if (strcmp(argv[1], "write") == 0 && argc == 4)
    {
      int val = atoi(argv[3]);
      if (gpioExtWrite(port_idx, pin_num, val) == true)
        cliPrintf("GPIO %c%d Set to %d\r\n", toupper(port_char), pin_num, val);
      else
        cliPrintf("Invalid Port or Pin (ex:a5, b12)\r\n");
    }
    else
    {
      cliPrintf("Usage: gpio read [a~h][0~15]\r\n");
      cliPrintf("       gpio write [a~h][0~15] [0|1]\r\n");
    }
  }
  else
  {
    cliPrintf("Usage: gpio read [a~h][0~15]\r\n");
    cliPrintf("       gpio write [a~h][0~15] [0|1]\r\n");
  }
}

static uint32_t led_toggle_period = 0;
void cliLed(uint8_t argc, char **argv)
{
  if (argc >= 2)
  {
    if (strcmp(argv[1], "on") == 0)
    {
      led_toggle_period=0;
      ledOn();
      LOG_INF("LED ON");
    }
    else if (strcmp(argv[1], "off") == 0)
    {
      led_toggle_period=0;
      ledOff();
      LOG_INF("LED OFF");
    }
    else if (strcmp(argv[1], "toggle") == 0)
    {
      if(argc==3){
        led_toggle_period=atoi(argv[2]);
        if(led_toggle_period>0) LOG_INF("LED  Auto-Toggled!!");
        else                    cliPrintf("Invalid Period\r\n");
      }
      else{
        led_toggle_period=0;
        ledToggle();
        LOG_INF("LED TOGGLE");
      }
    }
    else { cliPrintf("Invalid Command\r\n"); }
  }
  else
  {
    cliPrintf("Usage: led [on|off]\r\n");
    cliPrintf("     : led toggle\r\n");
    cliPrintf("     : led toggle [period]\r\n");
  }
}

void cliInfo(uint8_t argc, char **argv)
{
  if (argc == 1)
  {
    cliPrintf("===============================");
    cliPrintf("  HW Model   :  STM32F411\r\n");
    cliPrintf("  FW Version : V1.0.0\r\n");
    cliPrintf("  Build Date : %s %s\r\n", __DATE__, __TIME__);
    uint32_t uid0 = HAL_GetUIDw0();
    uint32_t uid1 = HAL_GetUIDw1();
    uint32_t uid2 = HAL_GetUIDw2();
    uint32_t dev  = HAL_GetDEVID();
    cliPrintf("  Serial Num : %08x-%08x-%08x\r\n", uid0, uid1, uid2);
    cliPrintf("  DevicID    : %08x\r\n", dev);
    cliPrintf("===============================\r\n");
  }
  else if (argc == 2 || strcmp(argv[1], "uptime") == 0)
  {
    cliPrintf("System Uptime: %d ms \r\n", millis());
  }
  else
  {
    cliPrintf("Usage: info\r\n");
    cliPrintf("       info [uptime]\r\n");
  }
}

void cliSys(uint8_t argc, char **argv)
{
  if ((argc == 2) && strcmp(argv[1], "reset") == 0)
    NVIC_SystemReset();
  else
    cliPrintf("Usage: sys [reset]\r\n");
}

static uint32_t temp_read_period = 0;
void cliTemp(uint8_t argc, char **argv){
  if(argc==1)
  {
    if(temp_read_period >0) tempStopAuto();
    temp_read_period=0;
    float t=tempReadSingle();
    cliPrintf("Current Temp: %.2f *C\r\n", t);
  }
  else if(argc==2){
    int period =atoi(argv[1]);
    if(period>0){
      tempStartAuto();
      temp_read_period=period;
      cliPrintf("Temperature Auto-Read Started (%d ms)\r\n",period);
    }
    else{
      tempStopAuto();
      cliPrintf("Invalid Period\r\n");
    }
  }
  else{
    tempStopAuto();
    cliPrintf("Usage: temp\r\n");
    cliPrintf("       temp [period]\r\n");
  }
}

void StartDefaultTask(void *argument)
{
  apInit();
  for(;;) { apMain(); }
}

/* ================================================================
 * Conveyor CLI
 * ================================================================ */
void cliConveyor(uint8_t argc, char **argv)
{
    if (argc < 2)
    {
        cliPrintf("Usage: conv start\r\n");
        cliPrintf("       conv stop\r\n");
        return;
    }

    if (strcmp(argv[1], "start") == 0)
    {
        Conveyor_Start(CONVEYOR_SLOW, CONVEYOR_DIR_BACKWARD);
        cliPrintf("Conveyor: START (pulse=1250)\r\n");
    }
    else if (strcmp(argv[1], "stop") == 0)
    {
        Conveyor_Stop();
        cliPrintf("Conveyor: STOP\r\n");
    }
    else
    {
        cliPrintf("Usage: conv [start|stop]\r\n");
    }
}

void ledSystemTask(void *argument)
{
  while (1)
  {
    if(led_toggle_period > 0){
      ledToggle();
      bool led_state=ledGetStatus();
      if(isMonitoringOn()) monitorUpdateValue(ID_OUT_LED_STATE,TYPE_BOOL,&led_state);
      else                 LOG_DBG("LED Toggle!");
      osDelay(led_toggle_period);
    }
    else{
      bool led_state=ledGetStatus();
      if(isMonitoringOn()) monitorUpdateValue(ID_OUT_LED_STATE,TYPE_BOOL,&led_state);
      osDelay(50);
    }
  }
}

void tempSystemTask(void *argument){
  while(1){
    if(temp_read_period>0){
      tempStartAuto();
      float t=tempReadAuto();
      if(isMonitoringOn()) monitorUpdateValue(ID_ENV_TEMP, TYPE_FLOAT, &t);
      else                 cliPrintf("Current Temp: %.2f *C\r\n", t);
      osDelay(temp_read_period);
    }
    else{ osDelay(50); }
  }
}

static uint32_t monitor_period = 0;
void monitorSystemTask(void *argument){
  while(1){
    if(isMonitoringOn()) monitorSendPacket();
    monitor_period=monitorGetPeriod();
    osDelay(monitor_period);
  }
}

void apStopAutoTask(void){
  monitorOff();
  led_toggle_period     = 0;
  temp_read_period      = 0;
  g_loadcell_auto_print = false;
  tempStopAuto();
  ledOff();
  Conveyor_Stop();
}

void apSyncPeriods(uint32_t period){
  if(period >0){
    tempStopAuto();
    temp_read_period=period;
    led_toggle_period=period;
    LOG_INF("Task Synchronized to %d ms", period);
  }
  else{
    temp_read_period=0;
    led_toggle_period=0;
  }
}

void armSystemTask(void *argument)
{
    LOG_INF("armSystemTask started!");
    while (1)
    {
        if (arm_running)
        {
            cliPrintf("[ARM] 시퀀스 시작\r\n");
            robotArmTask();
            arm_running = false;

            /* 시퀀스 완료 → systemTask에 세마포어 신호 */
            if (g_arm_done_sem != NULL)
            {
                osSemaphoreRelease(g_arm_done_sem);
                cliPrintf("[ARM] 시퀀스 완료 — 세마포어 release\r\n");
            }
            else
            {
                cliPrintf("[ARM] 경고: g_arm_done_sem NULL\r\n");
            }
        }
        else
        {
            osDelay(50);
        }
    }
}

/* ── CLI: system ─────────────────────────────────────────── */
void cliSystem(uint8_t argc, char **argv)
{
    if (argc < 2)
    {
        cliPrintf("Usage: system on\r\n");
        cliPrintf("       system off\r\n");
        cliPrintf("       system status\r\n");
        return;
    }

    if (strcmp(argv[1], "on") == 0)
    {
        if (g_system_enabled)
        {
            cliPrintf("[SYS] 이미 동작 중\r\n");
            return;
        }
        g_system_enabled = true;
        g_system_state   = STATE_IDLE;  /* systemTask 루프에서 WAIT_OBJECT로 전환 */
        cliPrintf("[SYS] 시스템 ON\r\n");
    }
    else if (strcmp(argv[1], "off") == 0)
    {
        g_system_enabled = false;
        arm_running      = false;
        Conveyor_Stop();
        Flag_SetCenter();
        cliPrintf("[SYS] 시스템 OFF — 컨베이어/팔 정지\r\n");
    }
    else if (strcmp(argv[1], "status") == 0)
    {
        const char *state_str[] = {
            "WAIT_OBJECT", "MEASURING", "ARM_MOVING",
            "CONVEYOR_RUNNING", "DONE", "IDLE"
        };
        cliPrintf("[SYS] enabled=%s  state=%s\r\n",
                  g_system_enabled ? "ON" : "OFF",
                  state_str[g_system_state]);
    }
    else
    {
        cliPrintf("Usage: system [on|off|status]\r\n");
    }
}

void apInit()
{
  hwInit();
  LOG_INF("Application Init... Started");

  logInit();
  monitorInit();
  Conveyor_Init();

  /* 세마포어 생성 — 로봇팔 완료 트리거용 */
  g_arm_done_sem = osSemaphoreNew(1, 0, NULL);

  monitorSetSyncHandler(apSyncPeriods);
  cliSetCtrlHandler(apStopAutoTask);

  cliAdd("led",       cliLed);
  cliAdd("info",      cliInfo);
  cliAdd("sys",       cliSys);
  cliAdd("gpio",      cliGpio);
  cliAdd("md",        cliMd);
  cliAdd("button",    cliButton);
  cliAdd("temp",      cliTemp);
  cliAdd("arm",       cliArm);
  cliAdd("conv",      cliConveyor);
  cliAdd("loadcell",  cliLoadCell);   /* ← LoadCell CLI 추가 */
  cliAdd("flag",      cliFlag);
  cliAdd("system",    cliSystem);     /* ← 시스템 on/off */

  if (pca9685Init() == true)
      LOG_INF("PCA9685 Init OK");
  else
      LOG_INF("PCA9685 Init FAIL");

  extern osThreadId_t myTaskArmHandle;
  if (myTaskArmHandle != NULL)
      LOG_INF("Arm task handle OK");
  else
      LOG_INF("Arm task handle NULL - creation FAILED");

  LOG_INF("systemTask — CubeMX freertos.c에서 생성됨");
}

void apMain(void)
{
  uartPrintf(0, "LED Task Started!!\r\n");
  while (1)
  {
    cliMain();
    osDelay(1);
  }
}