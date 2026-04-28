#ifndef MYAPP_AP_MONITOR_H_
#define MYAPP_AP_MONITOR_H_

#include "def.h"
#include "log_def.h"
#include "cli.h"

// ==========================================
// 프로토콜 정의
// ==========================================

#define MONITOR_STX 0x02
#define MONITOR_ETX 0x03

typedef enum {
    /* 0~9: 시스템 공통 상태 */
    ID_SYS_HEARTBEAT    = 0,    // 시스템 동작 여부 (Alive signal)
    ID_SYS_UPTIME       = 1,    // 시스템 가동 시간 (millis)
    ID_SYS_TEMP         = 2,    // MCU 내부 온도 센서
    ID_SYS_VREF         = 3,    // MCU 내부 참조 전압

    /* 10~29: 환경 센서 데이터 (주로 Float/Int) */
    ID_ENV_TEMP         = 10,   // 외부 온도 센서
    ID_ENV_HUMI         = 11,   // 외부 습도 센서
    ID_ENV_PRESS        = 12,   // 대기압 센서
    ID_ENV_LIGHT        = 13,   // 조도 센서 (Lux)

    /* 30~49: 사용자 입력 기기 (주로 Bool/Int) */
    ID_IN_BUTTON_1      = 30,   // 사용자 버튼 1 (Blue Button 등)
    ID_IN_BUTTON_2      = 31,   // 사용자 버튼 2
    ID_IN_SW_DIP        = 32,   // 딥 스위치 상태
    ID_IN_ENC_POS       = 33,   // 엔코더 회전 위치

    /* 50~69: 액추에이터 상태 피드백 (주로 Bool/Int) */
    ID_OUT_LED_STATE    = 50,   // LED On/Off 상태
    ID_OUT_MOTOR_SPEED  = 51,   // 모터 현재 속도 (RPM)
    ID_OUT_RELAY        = 52,   // 릴레이 작동 상태

    /* 70~89: 모션 및 위치 데이터 (주로 Float) */
    ID_IMU_ACCEL_X      = 70,   // 가속도 X축
    ID_IMU_ACCEL_Y      = 71,   // 가속도 Y축
    ID_IMU_ACCEL_Z      = 72,   // 가속도 Z축
    ID_IMU_GYRO_X       = 73,   // 자이로 X축
    
    /* 100+: 에러 및 알람 코드 */
    ID_ALARM_CRITICAL   = 100,  // 치명적 오류 코드
    ID_ALARM_WARN       = 101   // 단순 경고 코드
} SensorID;

typedef enum {
    TYPE_UINT8   = 0,
    TYPE_INT32   = 1,
    TYPE_FLOAT   = 2,
    TYPE_BOOL    = 3,
    TYPE_STRING  = 4  // 이번 구현에서는 4바이트 고정 길이를 위해 지원하지 않음
} DataType;

// 노드 데이터 구조 (6 Bytes)
// 02 01    0A 02 D6 2F C5 41  74 03
// 02 02    32 03 00 00 00 00  0A 02 FE C3 C7 41   80 03
// 02 02    32 03 01 00 00 00  0A 02 26 58 CA 41   CF 03
typedef struct __attribute__((packed)) {
    uint8_t id;
    uint8_t type;
    union {
        uint8_t  u8_val[4];
        int32_t  i_val;
        float    f_val;
        uint32_t u_val;
    } value;
} sensor_node_t;

#define MAX_SENSOR_NODES 20 // 최대로 보관할 수 있는 센서 종류 수


typedef struct {
    uint8_t count;
    sensor_node_t nodes[MAX_SENSOR_NODES];
} monitor_packet_t;


// ==========================================
// API 함수
// ==========================================

void monitorInit(void);
void monitorUpdateValue(SensorID id, DataType type, void *p_val);
void monitorSendPacket(void);

// 외부 모듈이 송신중단(Mute) 상태인지 확인하기 위한 Getter
bool isMonitoringOn(void);

void monitorOff(void);
uint32_t monitorGetPeriod(void);
void monitorSetPeriod(uint32_t period);

typedef void (*monitor_sync_cb_t)(uint32_t period);
void monitorSetSyncHandler(monitor_sync_cb_t handler);

#endif /* MYAPP_AP_MONITOR_H_ */
