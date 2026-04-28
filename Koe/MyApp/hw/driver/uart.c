#include "uart.h"
#include "cmsis_os2.h"
//#include "stm32f4xx_hal_uart.h"
#include <stdarg.h>
//#include <stdbool.h>
#include <stdio.h>
//#include "cmsis_os2.h"

extern UART_HandleTypeDef huart2;

static osMessageQueueId_t uart_rx_q = NULL;
static osMutexId_t uart_tx_mutex = NULL;

#define TIMEOUT 200

#define UART_RX_BUF_LENGTH 256

static uint8_t rx_buf[UART_RX_BUF_LENGTH];
static uint32_t rx_buf_head = 0;
static uint32_t rx_buf_tail = 0;
static uint8_t rx_data;




bool uartInit(void){

    if(uart_rx_q == NULL) {
        uart_rx_q = osMessageQueueNew(UART_RX_BUF_LENGTH, sizeof(uint8_t), NULL);
    }

    if(uart_tx_mutex == NULL) {
        uart_tx_mutex = osMutexNew(NULL);
    }

    bool ret = uartOpen(0,9600);
    HAL_UART_Receive_IT(&huart2, &rx_data, 1);

    return ret;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){

    if(huart->Instance==USART2){
        if(uart_rx_q != NULL){
            osMessageQueuePut(uart_rx_q, &rx_data, 0, 0);
        }
        HAL_UART_Receive_IT(&huart2, &rx_data, 1);
    }
}

uint32_t uartAvailable(uint8_t ch){
    if(ch == 0 && uart_rx_q != NULL){
        return osMessageQueueGetCount(uart_rx_q);
    }
    return 0;
}

uint8_t uartRead(uint8_t ch){
    uint8_t ret=0;
    if(ch == 0 && uart_rx_q != NULL){
        osMessageQueueGet(uart_rx_q, &ret, NULL, 0);
    }
    return ret;
}

bool uartReadBlock(uint8_t ch, uint8_t *p_data, uint32_t timeout) {
    if(ch==0 && uart_rx_q != NULL) {
        if(osMessageQueueGet(uart_rx_q, p_data, NULL, timeout) == osOK)
        return true;
    }
    return false;
}

bool uartOpen(uint8_t ch, uint32_t baudrate){
    
    if(huart2.Init.BaudRate!=baudrate)
       huart2.Init.BaudRate=baudrate;

    if(HAL_UART_DeInit(&huart2)!=HAL_OK)
        return false;

    if(HAL_UART_Init(&huart2)!=HAL_OK)
        return false;



    return true;
}

bool uartClose(uint8_t ch){
    return true;
}


uint32_t uartWrite(uint8_t ch, uint8_t *p_data, uint32_t len){

    if(uart_tx_mutex==NULL) return 0;
    osMutexAcquire(uart_tx_mutex, osWaitForever);

    if(HAL_UART_Transmit(&huart2,p_data,len, TIMEOUT)==HAL_OK){
    }

    else {
        len = 0;
    }
    
    osMutexRelease(uart_tx_mutex);
    return len;
}

uint32_t uartPrintf(uint8_t ch, const char *fmt, ...){

    char buf[128];
    uint32_t len;
    va_list args;

    va_start(args,fmt);

    len=vsnprintf(buf, 128, fmt, args);

    va_end(args);
    return uartWrite(ch,(uint8_t*)buf,len);
}
