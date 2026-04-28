#include "bsp.h"
#include "main.h"
#include "stm32f4xx_hal.h"

void bspInit(){}

void delay(uint32_t ms){
    HAL_Delay(ms);
}

uint32_t millis(void)
{
    return HAL_GetTick();
}
