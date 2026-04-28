#include "button.h"
#include "cli.h"



static bool is_enable=false;


void buttonInit(void){
    is_enable=false;
}
void buttonEnable(bool enable){
    is_enable=enable;
}
bool buttonGetEnable(void){
    return is_enable;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
    if(is_enable==false) return;
    if(GPIO_Pin==GPIO_PIN_13){
        cliPrintf("\r\n [Button] B1 pressed\r\n");
    }

}