# ⚖️ 무게 기반 자동 분류 시스템 (Weight-based Automatic Classification System)

STM32의 FreeRTOS를 활용한 로봇팔·컨베이어 벨트 통합 자동화 분류 프로젝트

---

## 📌 시스템 아키텍처

```
로드셀 (AD002 변환 모듈)
└─── 아날로그 신호 ───▶ STM32 F411RE (Nucleo)
                               │
                     ┌─────────┼─────────┐
                     │         │         │
                 이동 평균    컨베이어    5축 로봇팔
                 필터 처리    벨트 모터      피킹
                (노이즈 제거)  (이송 제어)  (분류 동작)
```

---

## 🛠 기술 스택

![C](https://img.shields.io/badge/C-A8B9CC?style=flat-square&logo=c&logoColor=white)
![C++](https://img.shields.io/badge/C++-00599C?style=flat-square&logo=cplusplus&logoColor=white)
![STM32](https://img.shields.io/badge/STM32-03234B?style=flat-square&logo=stmicroelectronics&logoColor=white)
![FreeRTOS](https://img.shields.io/badge/FreeRTOS-8CC84B?style=flat-square)
![Git](https://img.shields.io/badge/Git-F05032?style=flat-square&logo=git&logoColor=white)
![GitHub](https://img.shields.io/badge/GitHub-181717?style=flat-square&logo=github&logoColor=white)

---

## 🛠 사용 하드웨어

| 장치 | 역할 |
|---|---|
| STM32 F411RE (Nucleo) | 메인 MCU, FreeRTOS 실행, 전체 시퀀스 제어 |
| 로드셀 + HX711 | 물체 무게 아날로그 측정 및 디지털 변환 |
| PCA9685 | I2C 기반 16채널 PWM 서보 드라이버 |
| 5축 로봇팔 | 물체 피킹(Picking) 및 분류 동작 |
| 컨베이어 벨트 + 모터 | 물체 이송 |
| 플래그 서보모터 | 중량/경량 분류 방향 지시 |
| PIR 센서 | 물체 진입 감지 |
| 온도 센서 | 주변 환경 온도 수집 |
| LED | 시스템 상태 시각화 |

---

## 📡 통신 및 제어 인터페이스

### CLI 명령어 (UART)
```
monitor on / monitor off     # 원격 모니터링 스트리밍 ON / OFF
led period <ms>              # LED 점멸 주기 설정
temp period <ms>             # 온도 측정 주기 설정
status                       # 현재 시스템 상태 출력
```

### 모니터링 패킷 (UART 스트리밍)
```
WEIGHT:<g> TEMP:<°C> LED:<ON/OFF> STATE:<상태>
```

### 분류 기준
```
OBJECT_DETECT_G  이상 → 물체 감지 (측정 시작)
HEAVY 기준값     이상 → 중량(Heavy) → 플래그 Left + 로봇팔 Left 이송
HEAVY 기준값     미만 → 경량(Light) → 플래그 Right + 로봇팔 Right 이송
OBJECT_RELEASE_G 이하 → 물체 이탈 (대기 복귀)
```

### 주요 인터페이스
| 기능 | Peripheral | 연결 및 역할 |
|---|---|---|
| **로드셀 신호 입력** | ADC / GPIO | AD002 모듈과의 인터페이스 및 디지털 변환 데이터 수집 |
| **컨베이어 벨트 제어** | TIM PWM / GPIO | 모터 드라이버 방향 제어 및 PWM을 통한 속도 제어 |
| **5축 로봇팔 제어** | TIM PWM (Multi-Ch) | 각 관절 서보모터 제어를 위한 다채널 PWM 펄스 출력 |


---

## ⚙️ 주요 기능

### ⚖️ 정밀 무게 측정 및 노이즈 필터링
- 로드셀 + HX711을 통한 실시간 아날로그→디지털 무게 수집
- **중간값 필터 (Median Filter)**: 순간적인 스파이크 노이즈 제거
- **적응형 이동 평균 필터 (Adaptive EMA)**: 안정적인 무게 수렴값 산출
- **영점 자동 추적 (Zero Tracking)**: 잔여 오차 자동 보정으로 장시간 정밀도 유지

### 🤖 FSM 기반 자동화 공정 제어

| 상태 | 동작 |
|---|---|
| `STATE_INIT` | 로봇팔 초기 위치 정렬 |
| `STATE_WAIT_OBJECT` | 물체 투입 대기 |
| `STATE_MEASURING` | 무게 안정화 후 중량/경량 판별, 플래그 서보 구동 |
| `STATE_ARM_MOVING` | 로봇팔 분류 동작 완료까지 세마포어 대기 |
| `STATE_CONVEYOR_RUNNING` | 컨베이어 벨트 3초 구동 후 대기 상태 복귀 |

### 🦾 5축 로봇팔 부드러운 모션 제어
- PCA9685 I2C 서보 드라이버를 통한 다축 동시 제어
- `pca9685SetAngleSmooth()` 기반 보간 모션으로 자연스러운 피킹 동작 구현

### 📡 원격 모니터링 (텔레메트리)
- UART CLI를 통해 실시간 무게, 온도, LED 상태, 공정 상태 스트리밍
- GUI 모니터링 프로그램 연동 지원
- QT creator로 GUI와 UART CLI 연동

---

## 🔀 FreeRTOS 태스크 구성

> CMSIS-RTOS v2 (FreeRTOS wrapper) 기반, 총 4개의 독립 태스크 운용

| 태스크 | 주기 | 역할 |
|---|---|---|
| `loadCellSystemTask` | 250ms | 로드셀 데이터 수집, 필터링, 물체 감지/이탈 판단, 영점 추적 |
| `systemTask` | 50ms | FSM 기반 전체 공정 시퀀스 제어 |
| `armSystemTask` | 이벤트 구동 | 로봇팔 서보 모션 전담 (초기화 / 피킹 시퀀스) |
| `monitorSystemTask` | 가변 (CLI 설정) | 모니터링 패킷 외부 송신 |

### 💡 태스크 간 동기화 (Binary Semaphore)

```
systemTask                          armSystemTask
    │                                     │
    │  arm_running 플래그 SET              │
    │ ──────────────────────────────────▶ │  로봇팔 모션 수행
    │                                     │
    │  osSemaphoreAcquire(g_arm_done_sem) │
    │ ◀──────────────────────────────────── osSemaphoreRelease(g_arm_done_sem)
    │  (대기 해제 → 다음 공정 진행)         │  (모션 완료 후 세마포어 릴리즈)
```

→ CPU 자원 낭비 없이 RTOS의 블로킹 대기를 활용한 클린 멀티태스킹 구조

---

## 🔌 STM32 핀 설정

| 핀 | 기능 | 연결 대상 |
|---|---|---|
| PA0 | ADC or GPIO | HX711 DOUT (로드셀 데이터) |
| PA1 | GPIO_OUTPUT | HX711 SCK (로드셀 클럭) |
| PB6 | I2C1 SCL | PCA9685 SCL |
| PB7 | I2C1 SDA | PCA9685 SDA |
| PA2 | USART2 TX | CLI / 모니터링 UART TX |
| PA3 | USART2 RX | CLI / 모니터링 UART RX |
| PA5 | GPIO_OUTPUT | LED 상태 표시 |
| PB0 | GPIO_INPUT | PIR 센서 |
| TIM2 CH1 | PWM | 컨베이어 벨트 모터 드라이버 |
| TIM2 CH2 | PWM | 플래그 서보모터 (Left/Right) |

> ⚠️ 핀 정보는 STM32CubeIDE `.ioc` 파일 및 소스 코드 기준 — 정확한 정보는 `stm31411_cli/` 폴더 내 핀 설정 파일을 참고하세요.

---

## 🗄 파일 구조

```
Weight-based-automatic-classification-system/
├── stm31411_cli/              # STM32 메인 펌웨어 (STM32CubeIDE 프로젝트)
│   ├── Core/
│   │   ├── Src/
│   │   │   ├── main.c         # FreeRTOS 태스크 생성 및 시스템 초기화
│   │   │   ├── freertos.c     # RTOS 태스크 정의 (4개 태스크)
│   │   │   ├── loadcell.c     # 로드셀 측정 및 필터링 알고리즘
│   │   │   ├── arm.c          # PCA9685 로봇팔 모션 제어
│   │   │   ├── conveyor.c     # 컨베이어 벨트 PWM 제어
│   │   │   └── cli.c          # UART CLI 명령 처리
│   │   └── Inc/               # 헤더 파일 (.h)
│   └── stm31411_cli.ioc       # STM32CubeIDE 핀 설정 파일
├── Koe/                       # 팀원 개인 작업 폴더
├── stm31411_cli.zip           # 프로젝트 압축본
├── .gitignore
└── README.md
```
---

## 👥 팀 협업

- **인원**: 6명
- **형상 관리**: GitHub — 헤더(.h)와 소스(.c) 파일 분리로 모듈화, Merge 중심 코드 관리
- **개발 환경**: STM32CubeIDE
- **Notion**: [팀 협업 문서 바로가기](https://www.notion.so/Team-b97304a11b72836aa188015b787c27f1)

---

## 🌐 개발 환경

| 항목 | 버전 |
|---|---|
| STM32CubeIDE | 2.1.1 |
| CMSIS-RTOS v2 (FreeRTOS) | STM32Cube 내장 |
| HAL Library | STM32F4xx HAL |
| Target MCU | STM32 F411RE (Nucleo-64) |
| 언어 | C / C++ |
