#include "stm32f10x.h"                  // Device header
#include "Serial.h"

/* ═══════════════════════════════════════════════════
 *  Serial.c — USART2 串口驱动（指纹模块通信）
 *
 *  波特率 57600，8-N-1
 *  中断接收：RXNE逐字节接收 + IDLE空闲中断标志帧结束
 * ═══════════════════════════════════════════════════ */

/* ── 全局接收缓冲区 ────────────────────────────── */
unsigned char USART_ReceiveBuf[SERIAL_BUF_SIZE];
unsigned char USART_STA = 0;            // 接收完成标志
static unsigned char RxIndex = 0;       // 接收计数

/* ── Serial_Init ───────────────────────────────────
 * USART2: PA2=TX(AF_PP), PA3=RX(IPU), 57600-8-N-1
 * 中断：RXNE + IDLE
 * ────────────────────────────────────────────────── */
void Serial_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* 时钟使能 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	/* PA2 = TX (复用推挽输出) */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* PA3 = RX (上拉输入) */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* USART2 参数配置 */
	USART_InitStructure.USART_BaudRate = 57600;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART2, &USART_InitStructure);

	/* 中断使能 */
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);    // 接收中断
	USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);    // 空闲中断

	/* NVIC 配置 */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_Init(&NVIC_InitStructure);

	/* 使能 USART2 */
	USART_Cmd(USART2, ENABLE);
}

/* ── Serial_SendByte ────────────────────────────── */
void Serial_SendByte(unsigned char Byte)
{
	USART_SendData(USART2, Byte);
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}

/* ── Serial_SendArray ───────────────────────────── */
void Serial_SendArray(unsigned char *Array, unsigned int Length)
{
	unsigned int i;
	for (i = 0; i < Length; i++)
	{
		Serial_SendByte(Array[i]);
	}
}

/* ── USART2 中断服务函数 ───────────────────────────
 * RXNE：逐字节存入 USART_ReceiveBuf
 * IDLE：一帧结束，置位 USART_STA
 * ────────────────────────────────────────────────── */
void USART2_IRQHandler(void)
{
	unsigned char dr, sr;

	/* 接收中断 */
	if (USART_GetITStatus(USART2, USART_IT_RXNE) == SET)
	{
		dr = USART_ReceiveData(USART2);
		USART_ClearITPendingBit(USART2, USART_IT_RXNE);

		if (RxIndex < SERIAL_BUF_SIZE)
		{
			USART_ReceiveBuf[RxIndex++] = dr;
		}
	}

	/* 空闲中断（一帧接收完毕） */
	if (USART_GetITStatus(USART2, USART_IT_IDLE) == SET)
	{
		/* 清除 IDLE 标志（读SR+读DR） */
		sr = USART2->SR;
		dr = USART2->DR;

		USART_STA = 1;          // 通知主循环数据就绪
		RxIndex = 0;            // 指针归零，准备下一帧
	}
}