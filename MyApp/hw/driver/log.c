// ✅ log.c에 직접 추가
#include "log.h"
#include "hw_def.h"
#include "cli.h"
#include "uart.h"
#include <stdarg.h>
#include <stdio.h>    // vsnprintf 여기 있음


static uint8_t runtime_log_level = 3;

// log get set
void cliLog(uint8_t argc, char** argv)
{
    if(argc==2 && strcmp(argv[1], "get")==0)
    {
        cliPrintf("Current Log Level : %d\r\n", runtime_log_level);
    }
    else if(argc==3 && strcmp(argv[1], "set")==0)
    {
        uint8_t level = atoi(argv[2]);
        if(-1<level && level <= 5)
        {
            logSetLevel(level);
            cliPrintf("Log level set to %d\r\n", level);
        }
        else 
        {
            cliPrintf("Invaild level(0~5)\r\n");
        }
    }
    else 
    {
        cliPrintf("0:FATAL, 1:ERROR, 2:WARN, 3:INFO, 4:DEBUG, 5:VERBOSE\r\n");
        cliPrintf("Usage : log get\r\n");
        cliPrintf("        log set(0~5)\r\n");
    }
}

bool logInit(void)
{
    return true;
}
void logSetLevel(uint8_t level)
{
    runtime_log_level = level;
}
uint8_t logGetLevel(void)
{
    return runtime_log_level;
}

// public
uint8_t logGetRuntimeLevel(void)
{
    return runtime_log_level;
}
void logPrintf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    int len;

    va_start(args, fmt);
    len = vsnprintf(buf, 256, fmt, args);
    va_end(args);

    uartWrite(0,(uint8_t*)buf, len);
}