#include "stm32f10x.h"
#include "main.h"
#include "Serial.h"
#include "Delay.h"
#include "OLED.h"
#include "Key.h"
#include "Lock.h"
#include <string.h>

/* ═══════════════════════════════════════════════════
 *  Finger.c -- 指纹模块驱动（AS608 协议）
 *
 *  协议格式：EF01 [地址4B] [标识] [长度2B] [指令] [数据...] [校验和2B]
 *  通信方式：USART2，57600 波特率
 *  所有函数均含超时保护
 * ═══════════════════════════════════════════════════ */

#define BUF_SIZE      SERIAL_BUF_SIZE

/* ── 指令包 ───────────────────────────────────── */

static unsigned char PS_Cancel[12] =
    {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x30, 0x00, 0x34};

/* 读取模板数量 */
static unsigned char PS_ReadCount[12] =
    {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x1D, 0x00, 0x21};

/* 清空指纹库 */
static unsigned char PS_Empty[12] =
    {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x0D, 0x00, 0x11};

/* 自动注册（采集→生成特征→存储，一站式） */
static unsigned char PS_AutoEnroll[17] =
    {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x08, 0x31,
     '\0', '\0', 0x04, 0x00, 0x10, '\0', '\0'};

/* 自动验证（1:N 搜索） */
static unsigned char PS_AutoIdentify[17] =
    {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x08, 0x32,
     0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3C};

/* ── 内部辅助 ──────────────────────────────────── */

static void SendData(unsigned char *Array, unsigned int Length)
{
	unsigned int sum = 0;
	unsigned int i;
	for (i = 6; i < Length - 2; i++)
		sum += Array[i];
	Array[Length - 2] = (sum >> 8) & 0xFF;
	Array[Length - 1] =  sum       & 0xFF;
	Serial_SendArray(Array, Length);
}

static unsigned char Answer(unsigned char pos)
{
	if (pos < BUF_SIZE) return USART_ReceiveBuf[pos];
	return 0xFF;
}

/* WaitReply — 500×10ms=5s 超时，返回1=收到, 0=超时 */
static unsigned char WaitReply(void)
{
	unsigned int t = 500;
	while (!USART_STA && t > 0) { t--; delay_10ms(1); }
	return (USART_STA != 0);
}

/* ═══════════════════════════════════════════════════
 *  对外接口
 * ═══════════════════════════════════════════════════ */

void Finger_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(FINGER_PWR_CLK, ENABLE);
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin   = FINGER_PWR_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(FINGER_PWR_PORT, &GPIO_InitStructure);
	
	RCC_APB2PeriphClockCmd(FINGER_TOUCH_CLK, ENABLE);
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin   = FINGER_TOUCH_PIN;
	GPIO_Init(FINGER_TOUCH_PORT, &GPIO_InitStructure);

	FINGER_PWR_OFF;
	memset(USART_ReceiveBuf, 0xFF, BUF_SIZE);
}

/*
 * Finger_Unlock — 指纹开锁
 *   上电 → 循环发PS_AutoIdentify → 等待手按 → 比对
 *   最多30次（约9秒）超时退出
 */
void Finger_Unlock(void)
{
	unsigned char i;

	FINGER_PWR_ON;
	delay_10ms(15);

	for (i = 0; i < 30; i++)
	{
		memset(USART_ReceiveBuf, 0xFF, BUF_SIZE);
		USART_STA = 0;
		SendData(PS_AutoIdentify, 17);
		if (!WaitReply()) continue;

		if (Answer(8) == 0x08 && Answer(9) == 0x23)
		{
			OLED_Clear();
			OLED_ShowString(1, 1, "No fingerprint");
			delay_10ms(100); OLED_Clear();
			break;
		}
		if (Answer(9) == 0x26) continue;

		if (Answer(9) == 0x00 && Answer(10) == 0x05)
		{
			OLED_Clear();
			OLED_ShowString(1, 1, "Unlock OK");
			Lock_AccessGrant(100);
			OLED_Clear();
			break;
		}
		if (Answer(9) == 0x09 && Answer(10) == 0x05)
		{
			OLED_Clear();
			OLED_ShowString(1, 1, "Unlock Failed");
			Lock_AccessDeny();
			delay_10ms(100); OLED_Clear();
			break;
		}
	}

	if (i >= 30)
	{
		OLED_Clear();
		OLED_ShowString(1, 1, "Time out");
		delay_10ms(100); OLED_Clear();
	}

	FINGER_PWR_OFF;
}

/*
 * Finger_GetCount — 查询已录入数量（不上电，调用方自己管电源）
 */
static unsigned char ReadCount(void)
{
	unsigned char num = 0;
	memset(USART_ReceiveBuf, 0xFF, BUF_SIZE);
	USART_STA = 0;
	SendData(PS_ReadCount, 12);
	delay_10ms(3);
	if (Answer(6) == 0x07 && Answer(9) == 0x00)
		num = Answer(11);
	memset(USART_ReceiveBuf, 0xFF, BUF_SIZE);
	return num;
}

unsigned char Finger_GetCount(void)
{
	unsigned char cnt = 0;
	FINGER_PWR_ON;
	delay_10ms(15);
	cnt = ReadCount();
	FINGER_PWR_OFF;
	return cnt;
}

/*
 * Finger_Add — 添加一枚指纹
 *   自动查找当前已有数量，新ID = 数量 + 1
 *   无需手动输入ID
 */
void Finger_Add(void)
{
	unsigned char pageID, oldCnt;
	unsigned int t;

	FINGER_PWR_ON;
	delay_10ms(15);

	/* 查询当前已录入数量，新ID = 数量+1 */
	oldCnt = ReadCount();
	pageID = oldCnt + 1;
	if (pageID > 50) pageID = 1;    /* 满了从1覆盖 */

	/* 填入注册指令 */
	PS_AutoEnroll[10] = (pageID >> 8) & 0xFF;
	PS_AutoEnroll[11] =  pageID       & 0xFF;
	PS_AutoEnroll[15] = ((PS_AutoEnroll[10] + PS_AutoEnroll[11] + 0x4E) >> 8) & 0xFF;
	PS_AutoEnroll[16] =  (PS_AutoEnroll[10] + PS_AutoEnroll[11] + 0x4E)       & 0xFF;

	OLED_Clear();
	OLED_ShowString(1, 1, "Add Finger");
	OLED_ShowString(2, 1, "Place finger...");

	memset(USART_ReceiveBuf, 0xFF, BUF_SIZE);
	USART_STA = 0;
	SendData(PS_AutoEnroll, 17);
	if (!WaitReply()) goto AddFail;

	for (t = 0; t < 60; t++)
	{
		if (Answer(9) == 0x26) goto AddFail;               /* 超时 */
		if (Answer(9) == 0x22 || Answer(9) == 0x27)        /* ID或指纹已存在 */
		{
			OLED_Clear();
			OLED_ShowString(1, 1, "Already Exist!");
			delay_10ms(150); OLED_Clear();
			FINGER_PWR_OFF;
			return;
		}
		if (Answer(9) == 0x01) goto AddFail;               /* 注册失败 */

		if (Answer(9) == 0x00)                             /* 等待按手指 */
		{
			OLED_ShowString(1, 1, "Press Finger");
			if (Answer(10) == 0x03)
			{
				delay_10ms(50); OLED_Clear();
				OLED_ShowString(1, 1, "Press Finger");
			}
			if (Answer(11) == 0xF2)                        /* 注册成功 */
			{
				unsigned char newCnt = ReadCount();
				OLED_Clear();
				OLED_ShowString(1, 1, "Add Success!");
				OLED_ShowString(3, 1, "Total:");
				OLED_ShowNum(3, 8, newCnt, 3);
				LED_ON; BEEP_Beep(1, 20); delay_10ms(200); LED_OFF;
				FINGER_PWR_OFF;
				return;
			}
		}

		memset(USART_ReceiveBuf, 0xFF, BUF_SIZE);
		USART_STA = 0;
		SendData(PS_AutoEnroll, 17);
		if (!WaitReply()) break;
	}

AddFail:
	OLED_Clear();
	OLED_ShowString(1, 1, "Add Failed");
	LED_Blink(2, 5);
	delay_10ms(100); OLED_Clear();
	FINGER_PWR_OFF;
}

/*
 * Finger_ClearAll — 清空指纹库（删除Flash中全部模板）
 */
void Finger_ClearAll(void)
{
	FINGER_PWR_ON;
	delay_10ms(15);

	memset(USART_ReceiveBuf, 0xFF, BUF_SIZE);
	USART_STA = 0;
	SendData(PS_Empty, 12);
	if (!WaitReply())
	{
		OLED_Clear();
		OLED_ShowString(1, 1, "No Response");
		delay_10ms(100); OLED_Clear();
		FINGER_PWR_OFF;
		return;
	}

	if (Answer(9) == 0x00)
	{
		OLED_Clear();
		OLED_ShowString(1, 1, "All Cleared!");
		LED_ON; BEEP_Beep(1, 20); delay_10ms(150); LED_OFF;
	}
	else
	{
		OLED_Clear();
		OLED_ShowString(1, 1, "Clear Failed");
		LED_Blink(2, 5);
		delay_10ms(100);
	}
	OLED_Clear();
	FINGER_PWR_OFF;
}
