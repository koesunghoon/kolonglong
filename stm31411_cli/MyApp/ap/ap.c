#include "ap.h"
#include "pca9685.h"

/* ═══════════════════════════════════════════════════════════
 *  LoadCell Task
 * ═══════════════════════════════════════════════════════════ */

/* ── LoadCell 설정 ───────────────────────────────────────── */
#define LOADCELL_TASK_PERIOD_MS    250
#define TARE_SAMPLE_COUNT          20
#define HX711_READY_TIMEOUT_MS     500

/* 하드코딩 기준값 — loadcell rawtare 로 확인 후 교체 */
#define DEFAULT_TARE_OFFSET        (-583800)

/* 캘리브레이션 계수 */
#define SCALE_FACTOR_DEFAULT       40.09f

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

/* raw 안정 판단 임계값 */
#define RAW_STABLE_THRESH          300

/* 자동 tare 주기 (ms) */
#define AUTO_TARE_INTERVAL_MS      5000

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
static float   g_stable_last   = 0.0f;
static uint8_t g_stable_count  = 0;
static float   g_stable_weight = 0.0f;

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
void loadCellSystemTask(void *argument)
{
    int32_t  prev_raw          = 0;
    int32_t  raw               = 0;
    bool     obj_ever_detected = false;
    bool     tare_done         = false;
    uint32_t no_obj_start_ms   = 0;

    hx711Init(HX711_GAIN_128);
    LOG_INF("HX711 Init OK");

    osDelay(500);
    runTare();
    prev_raw        = g_tare_offset;
    no_obj_start_ms = osKernelGetTickCount();

    for (;;)
    {
        if (g_tare_request)
        {
            g_tare_request    = false;
            runTare();
            prev_raw          = g_tare_offset;
            tare_done         = true;
            obj_ever_detected = false;
        }

        if (!readRaw(&raw))
        {
            LOG_WRN("HX711 timeout");
            osDelay(LOADCELL_TASK_PERIOD_MS);
            continue;
        }

        int32_t delta = raw - prev_raw;
        if (delta < 0) delta = -delta;
        prev_raw = raw;

        float weight = calcWeight(raw);
        updateStable(weight);

        bool obj_detected = (g_stable_weight > DEADBAND_GRAM);

        if (obj_detected)
        {
            obj_ever_detected = true;
            tare_done         = false;
            no_obj_start_ms   = 0;
        }
        else
        {
            if (no_obj_start_ms == 0)
                no_obj_start_ms = osKernelGetTickCount();

            uint32_t elapsed = osKernelGetTickCount() - no_obj_start_ms;

            if (obj_ever_detected &&
                !tare_done &&
                (delta < RAW_STABLE_THRESH) &&
                (elapsed >= AUTO_TARE_INTERVAL_MS))
            {
                cliPrintf("[AutoTare] 자동 tare 실행\r\n");
                runTare();
                prev_raw  = g_tare_offset;
                tare_done = true;
            }
        }

        if (g_loadcell_auto_print)
        {
            cliPrintf("[LoadCell] raw=%ld  weight=%.2f g\r\n",
                      (long)raw, g_stable_weight);
        }

        osDelay(LOADCELL_TASK_PERIOD_MS);
    }
}

/* ═══════════════════════════════════════════════════════════
 *  이하 기존 ap.c 코드 100% 유지
 * ═══════════════════════════════════════════════════════════ */

static volatile bool arm_running = false;

void robotArmTask(void) {
    if (!arm_running) return;
    
// 1. 집게 열고
    pca9685SetAngleSmoothDual(0, 0, 1, 0, 1500);
    if (!arm_running) return;

    // 2. 고개 초기화
    pca9685SetAngleSmooth(2, 90 , 1500);
    if (!arm_running) return;
    
    // 3. 팔 내리고
    pca9685SetAngleSmooth(3, 140, 1500);
    if (!arm_running) return;

    // 4. 어깨 내리고
    pca9685SetAngleSmooth(4, 0, 1500);
    if (!arm_running) return;

    // 5. 집게 닫고
    pca9685SetAngleSmoothDual(0, 120, 1, 120, 1500);
    if (!arm_running) return;

    // 6. 어깨 올리고
    pca9685SetAngleSmooth(4, 80, 1500);
    if (!arm_running) return;

    // 7. 허리 돌리고
    pca9685SetAngleSmooth(5, 140, 1500);
    if (!arm_running) return;

    // 8. 어깨 내리고
    pca9685SetAngleSmooth(4, 0, 1500);
    if (!arm_running) return;

    // 9. 집게 열고
    pca9685SetAngleSmoothDual(0, 0, 1, 0, 1500);

    // 10. 허리 원위치
    pca9685SetAngleSmooth(5, 55, 1500);
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

void armSystemTask(void *argument) {
  LOG_INF("armSystemTask started!");
  while (1) {
    if (arm_running) {
      LOG_INF("arm running!");
      robotArmTask();
    } else {
      osDelay(50);
    }
  }
}

void apInit(void)
{
  hwInit();
  LOG_INF("Application Init... Started");

  logInit();
  monitorInit();
  Conveyor_Init();

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
  cliAdd("flag", cliFlag);

  if (pca9685Init() == true)
      LOG_INF("PCA9685 Init OK");
  else
      LOG_INF("PCA9685 Init FAIL");

      extern osThreadId_t myTaskArmHandle;
  if (myTaskArmHandle != NULL)
      LOG_INF("Arm task handle OK");
  else
      LOG_INF("Arm task handle NULL - creation FAILED");
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