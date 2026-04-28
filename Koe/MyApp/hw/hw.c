#include "driver/button.h"
#include "driver/temp.h"
#include "hw_def.h"
#include "hw.h"
#include "cli.h"
#include "uart.h"



void hwInit(void)
{
    ledInit();
    uartInit();
    cliInit();
    buttonInit();
    tempInit();
}
