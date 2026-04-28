#include "log.h"
#include "led.h"

static uint8_t runtime_log_level = 3;

void clilog(uint8_t argc, char** argv) {
    if(argc == 2 && strcmp(argv[1], "get") == 0) {
        cliPrintf("Current Log Level: %d\r\n", runtime_log_level);
    }
    else if(argc == 3 && strcmp(argv[1], "set") == 0) {
        uint8_t level = atoi(argv[2]);
        if (0 <= level && level <= 5) {
            logSetLevel(level);
            cliPrintf("Log Level Set to %d\r\n", level);
            
        }
        else {
            cliPrintf("Invalid level(0~5)\r\n");
        }
    }
    else {
        cliPrintf("0:FATAL, 1:ERROR, 2:WARN, 3:INFO, 4:DEBUG, 5:VERBOSE\r\n");
        cliPrintf("Usage: log get\r\n");
        cliPrintf("       log set [0~5]\r\n");
    }
}

bool logInit(void) {
    cliAdd("log", clilog);
    return true;
}

void logSetLevel(uint8_t level) {
    runtime_log_level = level;
}

uint8_t logGetLevel(void) {
    return runtime_log_level;
}

//public
uint8_t logGetRuntimeLevel(void) {
    
}

void logPrintf(const char *fmt, ...) {
    char buf[256];
    va_list args;
    int len;

    va_start(args, fmt);
    len = vsnprintf(buf, 256, fmt, args);
    va_end(args);

    uartWrite(0, (uint8_t*)buf, len);
}
