#include "temp.h"
#include "adc.h"
#include "cmsis_os2.h"
#include "stm32f4xx_hal_adc.h"
#include "stm32f4xx_hal_def.h"

static volatile uint32_t adc_dma_buf[1];

bool tempInit()
{
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_dma_buf, 1);
    return true;
}
float tempReadAuto()
{
    uint32_t adc_val = adc_dma_buf[0];

    float vsense = (adc_val/4095.0f)*3.3f;
    float temp_celsius=((vsense-0.76f)/0.0025f)+25.0f;

    return temp_celsius;
}

float tempReadSingle()
{
    uint32_t adc_val = 0;
    tempStartAuto();
    osDelay(100);
    adc_val = adc_dma_buf[0];
    tempStopAuto();
    
    float vsense = (adc_val/4095.0f)*3.3f;
    float temp_celsius=((vsense-0.76f)/0.0025f)+25.0f;
    return temp_celsius;
}

void tempStartAuto()
{
   HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma_buf, 1);
}

void tempStopAuto()
{
    HAL_ADC_Stop_DMA(&hadc1);
}
