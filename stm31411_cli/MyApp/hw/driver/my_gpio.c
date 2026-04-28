#include "my_gpio.h"
#include "stm32f411xe.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_rcc.h"

static GPIO_TypeDef* getPortPtr(uint8_t port_idx)
{
    switch(port_idx)
    {
        case 0 : __HAL_RCC_GPIOA_CLK_ENABLE();
         return GPIOA;
        case 1 : __HAL_RCC_GPIOB_CLK_ENABLE();
        return GPIOB;
        case 2 : __HAL_RCC_GPIOC_CLK_ENABLE();
        return GPIOC;
        case 3 : __HAL_RCC_GPIOD_CLK_ENABLE();
        return GPIOD;
        case 4 : __HAL_RCC_GPIOE_CLK_ENABLE();
        return GPIOE;
        // case 5 : return GPIOF;
        // case 6 : return GPIOG;
        case 7 : __HAL_RCC_GPIOH_CLK_ENABLE();
        return GPIOH;
        default : return NULL;
    }
}

bool gpioExtWrite(uint8_t port_idx, uint8_t pin_num, uint8_t state)
{
    if(pin_num>15) return false;

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = (1 << pin_num);
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(getPortPtr(port_idx), &GPIO_InitStruct);

    GPIO_TypeDef* pPort = getPortPtr(port_idx);
    if(pPort == NULL) return false;

    uint16_t pin_mask=(1<<pin_num);
    HAL_GPIO_WritePin(pPort,pin_mask, (state > 0)?GPIO_PIN_SET:GPIO_PIN_RESET);
    return true;
}

int8_t gpioExtRead(uint8_t port_idx, uint8_t pin_num)
{
     if(pin_num>15) 
     return -1;  // 1:high, 0:low, 그 외는 error 

     GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = (1 << pin_num);
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(getPortPtr(port_idx), &GPIO_InitStruct);

    GPIO_TypeDef* pPort = getPortPtr(port_idx);
    if(pPort == NULL) return -1;     

    uint16_t pin_mask=(1<<pin_num);
    return HAL_GPIO_ReadPin(pPort, pin_mask) == GPIO_PIN_SET?1:0;
}