#include "hw_def.h"
#include "main.h"
#include "stm32f411xe.h"
#include "stm32f4xx_hal_gpio.h"
void ledInit(void)
{

}

void ledOn(void)
{
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
}

void ledOff(void)
{
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
}

void ledToggle(void)
{
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
}

bool ledGetStatus(void)
{
    return(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5)==GPIO_PIN_SET)?true:false;
}