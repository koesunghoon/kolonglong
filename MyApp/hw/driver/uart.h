#ifndef __HW_DRIVER_UART_H__
#define __HW_DRIVER_UART_H__

#include "hw_def.h"
#include "usart.h"

bool uartInit(void);
bool uartOpen(uint8_t ch, uint32_t baudrate);
bool uartClose(uint8_t ch);
uint32_t uartWrite(uint8_t ch, uint8_t *p_data, uint32_t len);
uint32_t uartAvailable(uint8_t ch);
uint8_t uartRead(uint8_t ch);
uint32_t uartPrintf(uint8_t ch, const char *fmt, ...);

bool uartReadBlock(uint8_t ch, uint8_t *p_data, uint32_t timeout);
bool uartWriteBlock(uint8_t ch, uint8_t *p_data, uint32_t len);


#endif