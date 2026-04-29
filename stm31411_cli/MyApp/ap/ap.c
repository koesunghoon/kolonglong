#include "ap.h"
#include "pca9685.h"




static volatile bool arm_running = false;

void robotArmTask(void) {
    if (!arm_running) return;

    // 1. 집게 열고
    pca9685SetAngleSmoothDual(0, 0, 1, 0, 1000);
    if (!arm_running) return;

    // 2. 팔 내리고
    pca9685SetAngleSmooth(2, 120, 1000);
    if (!arm_running) return;

    // 3. 어깨 내리고
    pca9685SetAngleSmooth(3, 0, 1000);
    if (!arm_running) return;

    // 4. 집게 닫고
    pca9685SetAngleSmoothDual(0, 120, 1, 120, 1000);
    if (!arm_running) return;

    // 5. 어깨 올리고
    pca9685SetAngleSmooth(3, 80, 1000);
    if (!arm_running) return;

    // 6. 허리 돌리고
    pca9685SetAngleSmooth(4, 120, 1000);
    if (!arm_running) return;

    // 7. 어깨 내리고
    pca9685SetAngleSmooth(3, 0, 1000);
    if (!arm_running) return;

    // 8. 집게 열고
    pca9685SetAngleSmoothDual(0, 0, 1, 0, 1000);

    // 9. 허리 원위치 ← 추가
    pca9685SetAngleSmooth(4, 80, 1000);
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
  if (0x08000000 <= addr && addr <= 0x0807FFFF)
    return true;

  // 2. f411 ram
  if (0x20000000 <= addr && addr <= 0x20001FFF)
    return true;

  // 3. system memory
  if (0x1FFF0000 <= addr && addr <= 0x1FFF7A1F)
    return true;

  // 4. Peripheral register
  if (0x40000000 <= addr && addr <= 0x5FFFFFFF)
    return true;

  return false;
}

// md 0x8000-0000 32
void cliMd(uint8_t argc, char **argv)
{
  if (argc >= 2)
  {
    uint32_t addr = strtoul(argv[1], NULL, 16);
    uint32_t length = 16;

    if (argc >= 3)
    {
      length = strtoul(argv[2], NULL, 0);
    }

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
          else
          {
            cliPrintf("Not valid address!!\r\n");
            break;
          }
        }
        else
        {
          cliPrintf("   ");
        }
      }
      //
      cliPrintf(" | ");

      for (uint32_t j = 0; j < 16; j++)
      {
        if (i + j < length)
        {
          uint32_t target_addr = (addr + i + j);
          if (isSafeAddress(target_addr))
          {
            uint8_t val = *((volatile uint8_t *)target_addr);
            if (val >= 0x20 && val <= 0x7E)
            {
              cliPrintf("%c", val);
            }
            else
            {
              cliPrintf(".");
            }
          }
          else
          {
            cliPrintf("Not valid address!!\r\n");
            break;
          }
        }
      }

      cliPrintf("\r\n");
    }
  }
  else
  {
    cliPrintf("Usage : md [add(hex)] [length]\r\n");
    cliPrintf("        md 08000000 32 \r\n");
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
      if (state < 0)
      {
        cliPrintf("Invalid Port or Pin (ex:a5, b12)\r\n");
      }
      else
      {
        cliPrintf("GPIO %c%d=%d\r\n", toupper(port_char), pin_num, state);
      }
    }
    else if (strcmp(argv[1], "write") == 0 && argc == 4)
    {
      int val = atoi(argv[3]);
      if (gpioExtWrite(port_idx, pin_num, val) == true)
      {
        cliPrintf("GPIO %c%d Set to %d\r\n", toupper(port_char), pin_num, val);
      }
      else
      {
        cliPrintf("Invalid Port or Pin (ex:a5, b12)\r\n");
      }
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
        if(led_toggle_period>0){
          LOG_INF("LED  Auto-Toggled!!");
        }
        else{
          cliPrintf("Invalid Period\r\n");
        }

      }
      else{
        led_toggle_period=0;
        ledToggle();
        LOG_INF("LED TOGGLE");
      }
    }
    else
    {
      cliPrintf("Invalid Command\r\n");
    }
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
    uint32_t dev = HAL_GetDEVID();

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
  {
    NVIC_SystemReset();
  }
  else
  {
    cliPrintf("Usage: sys [reset]\r\n");
  }
}
static uint32_t temp_read_period = 0;
void cliTemp(uint8_t argc, char **argv){
  if(argc==1)
  {
    if(temp_read_period >0){
      tempStopAuto();
    }
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
  for(;;)
  {
    apMain();
  }
  
}

/* ================================================================
 * Conveyor CLI
 *
 * conv start [speed] [dir]   speed: 1~4 / dir: f(orward)|b(ackward)
 * conv stop
 * conv speed [1~4]
 * conv dir
 * conv status
 *
 * 예시:
 *   conv start 2 f     → MEDIUM 속도 전진
 *   conv start 3 b     → FAST 속도 후진
 *   conv speed 4       → TURBO로 속도 변경
 *   conv dir           → 방향 반전
 *   conv stop          → 정지
 *   conv status        → 현재 상태 출력
 * ================================================================ */
void cliConveyor(uint8_t argc, char **argv)
{
  if (argc < 2)
  {
    cliPrintf("Usage: conv start [speed 1~4] [f|b]\r\n");
    cliPrintf("       conv stop\r\n");
    cliPrintf("       conv speed [1~4]\r\n");
    cliPrintf("       conv dir\r\n");
    cliPrintf("       conv status\r\n");
    return;
  }
 
  /* conv stop */
  if (strcmp(argv[1], "stop") == 0)
  {
    Conveyor_Stop();
    cliPrintf("Conveyor: STOPPED\r\n");
    LOG_INF("Conveyor stopped");
  }
 
  /* conv start [speed] [dir] */
  else if (strcmp(argv[1], "start") == 0)
  {
    ConveyorSpeed_t spd = CONVEYOR_MEDIUM;
    ConveyorDir_t   dir = CONVEYOR_DIR_FORWARD;
 
    if (argc >= 3)
    {
      int s = atoi(argv[2]);
      if (s >= 1 && s <= 4) spd = (ConveyorSpeed_t)s;
      else { cliPrintf("Speed must be 1~4\r\n"); return; }
    }
    if (argc >= 4)
    {
      if (argv[3][0] == 'b' || argv[3][0] == 'B')
        dir = CONVEYOR_DIR_BACKWARD;
    }
 
    Conveyor_Start(spd, dir);
    cliPrintf("Conveyor: START  speed=%d  dir=%s\r\n",
              spd, (dir == CONVEYOR_DIR_FORWARD) ? "FORWARD" : "BACKWARD");
    LOG_INF("Conveyor started spd=%d dir=%d", spd, dir);
  }
 
  /* conv speed [1~4] */
  else if (strcmp(argv[1], "speed") == 0)
  {
    if (argc < 3) { cliPrintf("Usage: conv speed [1~4]\r\n"); return; }
    int s = atoi(argv[2]);
    if (s < 1 || s > 4) { cliPrintf("Speed must be 1~4\r\n"); return; }
    Conveyor_SetSpeed((ConveyorSpeed_t)s);
    cliPrintf("Conveyor: speed set to %d\r\n", s);
  }
 
  /* conv dir  (토글) */
  else if (strcmp(argv[1], "dir") == 0)
  {
    Conveyor_ToggleDirection();
    const ConveyorState_t *st = Conveyor_GetState();
    cliPrintf("Conveyor: direction -> %s\r\n",
              (st->direction == CONVEYOR_DIR_FORWARD) ? "FORWARD" : "BACKWARD");
  }
 
  /* conv status */
  else if (strcmp(argv[1], "status") == 0)
  {
    const ConveyorState_t *st = Conveyor_GetState();
    cliPrintf("---- Conveyor Status ----\r\n");
    cliPrintf("  Running  : %s\r\n",  st->running   ? "YES" : "NO");
    cliPrintf("  Speed    : %d\r\n",  st->speed);
    cliPrintf("  Dir      : %s\r\n",  (st->direction == CONVEYOR_DIR_FORWARD) ? "FORWARD" : "BACKWARD");
    cliPrintf("  Pulse    : %lu us\r\n", st->pulse_us);
    cliPrintf("-------------------------\r\n");
  }

  /* conv pulse [us] */
  else if (strcmp(argv[1], "pulse") == 0)
  {
      if (argc < 3) { cliPrintf("Usage: conv pulse [500~2500]\r\n"); return; }
      uint32_t us = (uint32_t)atoi(argv[2]);
      Conveyor_SetPulse(us);
      cliPrintf("Pulse set to %lu us\r\n", us);
  }
 
  else
  {
    cliPrintf("Unknown command. Try: conv start/stop/speed/dir/status\r\n");
  }

}

void ledSystemTask(void *argument)
{
  while (1)
  {
    if(led_toggle_period > 0){
      
      ledToggle();
      bool led_state=ledGetStatus();

      if(isMonitoringOn()){
        monitorUpdateValue(ID_OUT_LED_STATE,TYPE_BOOL,&led_state);
      }
      else{
        LOG_DBG("LED Toggle!");
      }
      
      osDelay(led_toggle_period);
    }
    else{
      bool led_state=ledGetStatus();
      if(isMonitoringOn()){
        monitorUpdateValue(ID_OUT_LED_STATE,TYPE_BOOL,&led_state);
      }
      osDelay(50);
    }
  }
}

void tempSystemTask(void *argument){
  while(1){
    if(temp_read_period>0){
      tempStartAuto();
      float t=tempReadAuto();
      if(isMonitoringOn()){
        monitorUpdateValue(ID_ENV_TEMP, TYPE_FLOAT, &t);
      }
      else{
        cliPrintf("Current Temp: %.2f *C\r\n", t);
      }
      osDelay(temp_read_period);
    }
    else{
      osDelay(50);
    }

  }
}
static uint32_t monitor_period = 0;
void monitorSystemTask(void *argument){
  while(1){
    if(isMonitoringOn()){
      monitorSendPacket();
    }
    monitor_period=monitorGetPeriod();
    osDelay(monitor_period);
  }

}


void apStopAutoTask(void){
  monitorOff();
  led_toggle_period=0;
  temp_read_period=0;
  tempStopAuto();
  ledOff();
  Conveyor_Stop();  // 시스템 정지 시 컨베이어도 함께 정지
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


  cliAdd("led", cliLed);
  cliAdd("info", cliInfo);
  cliAdd("sys", cliSys);
  cliAdd("gpio", cliGpio);
  cliAdd("md", cliMd);
  cliAdd("button", cliButton);
  cliAdd("temp", cliTemp);
  cliAdd("arm", cliArm);  // ← 추가
  cliAdd("conv",   cliConveyor); // 컨베이어 CLI 등록

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
    osDelay(1);  // ← 이거 추가, 다른 task에게 CPU 양보
  }
}
