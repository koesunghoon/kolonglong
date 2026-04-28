#include "driver/uart.h"
#include "led.h"
#include "cli.h"
#include "uart.h"
#include "button.h" 
#include "temp.h"     

void hwInit(void)
{
    ledInit();
    uartInit();
    cliInit();
    buttonInit();
    tempInit();
}