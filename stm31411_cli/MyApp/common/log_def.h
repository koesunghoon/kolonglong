#ifndef MYAPP_COMMON_LOG_H_
#define MYAPP_COMMON_LOG_H_

#include "def.h"
#include "bsp.h" // millis() 사용을 위함

// 1. 로그 레벨 정의 (0 ~ 5)
#define LOG_LEVEL_FATAL   0
#define LOG_LEVEL_ERROR   1
#define LOG_LEVEL_WARN    2
#define LOG_LEVEL_INFO    3
#define LOG_LEVEL_DEBUG   4
#define LOG_LEVEL_VERBOSE 5

// 2. 컴파일 타임 최대 로그 레벨 (바이너리 크기 최적화용)
#ifndef MAX_LOG_LEVEL
#define MAX_LOG_LEVEL    LOG_LEVEL_VERBOSE
#endif

// 3. 모듈별 태그 설정 (파일 상단에 정의하지 않으면 기본값 SYS)
#ifndef LOG_TAG
#define LOG_TAG "SYS"
#endif

// 4. ANSI 색상 코드
#define LOG_CLR_RESET  "\033[0m"
#define LOG_CLR_FATAL  "\033[0;35m" // PURPLE
#define LOG_CLR_ERROR  "\033[0;31m" // RED
#define LOG_CLR_WARN   "\033[0;33m" // YELLOW
#define LOG_CLR_INFO   "\033[0;32m" // GREEN
#define LOG_CLR_DEBUG  "\033[0;36m" // CYAN
#define LOG_CLR_VERB   "\033[0;37m" // WHITE

// 런타임 로그 레벨 제어 함수 (log.c에서 구현)
uint8_t logGetRuntimeLevel(void);
void    logPrintf(const char *fmt, ...); // cliPrintf 대용

// 로그 출력 핵심 매크로
#define LOG_PRINT(level, color, level_str, fmt, ...) \
    do { \
        if (MAX_LOG_LEVEL >= level && logGetRuntimeLevel() >= level) { \
            logPrintf("[%8d][%s][%s%s%s] " fmt "\r\n", \
                      millis(), LOG_TAG, color, level_str, LOG_CLR_RESET, ##__VA_ARGS__); \
        } \
    } while(0)

// 5. 공개 로깅 매크로
#define LOG_FTL(fmt, ...) LOG_PRINT(LOG_LEVEL_FATAL, LOG_CLR_FATAL, "FATAL", fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...) LOG_PRINT(LOG_LEVEL_ERROR, LOG_CLR_ERROR, "ERROR", fmt, ##__VA_ARGS__)
#define LOG_WRN(fmt, ...) LOG_PRINT(LOG_LEVEL_WARN, LOG_CLR_WARN,  "WARN ", fmt, ##__VA_ARGS__)
#define LOG_INF(fmt, ...) LOG_PRINT(LOG_LEVEL_INFO, LOG_CLR_INFO,  "INFO ", fmt, ##__VA_ARGS__)
#define LOG_DBG(fmt, ...) LOG_PRINT(LOG_LEVEL_DEBUG, LOG_CLR_DEBUG, "DEBUG", fmt, ##__VA_ARGS__)
#define LOG_VER(fmt, ...) LOG_PRINT(LOG_LEVEL_VERBOSE, LOG_CLR_VERB,  "VERB ", fmt, ##__VA_ARGS__)

#endif /* MYAPP_COMMON_LOG_H_ */
