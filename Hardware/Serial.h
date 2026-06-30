#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"

/* ── 串口接收缓冲区 ────────────────────────────── */
#define SERIAL_BUF_SIZE     40
extern unsigned char USART_ReceiveBuf[SERIAL_BUF_SIZE];
extern unsigned char USART_STA;     // 接收完成标志（空闲中断置1）

void Serial_Init(void);
void Serial_SendByte(unsigned char Byte);
void Serial_SendArray(unsigned char *Array, unsigned int Length);

#endif