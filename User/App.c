#include "stm32f10x.h"
#include "OLED.h"
#include "main.h"
#include "mfrc522.h"
#include "Key.h"
#include "Admin.h"
#include "App.h"
#include <string.h>

/* ═══════════════════════════════════════════════════
 *  App.c — 常态模式
 *
 *  开锁：IC卡 / 4位密码
 *  管理：长按 # 1.5秒
 *  密码：按任意数字键 → 该键作为第1位 → 继续输后3位
 * ═══════════════════════════════════════════════════ */

#define MAX_CARD_COUNT      5
#define PASSWORD_LENGTH     4
#define MAX_PW_RETRY        3

extern unsigned char WhiteList[MAX_CARD_COUNT][4];
extern unsigned char CardCount;
extern unsigned char AdminPassword[PASSWORD_LENGTH];

static unsigned int HoldTime = 0;

static unsigned char CheckCard(unsigned char *snr)
{
	unsigned char i;
	for (i = 0; i < CardCount; i++)
	{
		if (memcmp(WhiteList[i], snr, 4) == 0)
			return 1;
	}
	return 0;
}

static void LED_Blink(unsigned char times, unsigned int dly)
{
	unsigned char i;
	for (i = 0; i < times; i++)
	{
		LED_ON;  delay_10ms(dly);
		LED_OFF; if (i < times - 1) delay_10ms(dly);
	}
}

static unsigned char IsDigitKey(unsigned char key)
{
	return (key <= 2 || key == 4 || key == 5 || key == 6 ||
	        key == 8 || key == 9 || key == 10 || key == 13);
}

static unsigned char KeyToDigit(unsigned char key)
{
	if (key == 13) return 0;
	if (key == 0)  return 1;
	if (key == 1)  return 2;
	if (key == 2)  return 3;
	if (key == 4)  return 4;
	if (key == 5)  return 5;
	if (key == 6)  return 6;
	if (key == 8)  return 7;
	if (key == 9)  return 8;
	if (key == 10) return 9;
	return 0;
}

/* ── 密码开锁 ────────────────────────────────────
 * firstDigit: 触发密码输入的数字键值(0xFF=无，内部触发)
 *             为数字键值时直接作为第1位密码
 * ──────────────────────────────────────────────── */
static void App_PasswordUnlock(unsigned char firstKey)
{
	unsigned char buf[PASSWORD_LENGTH];
	unsigned char pos = 0, key;

	OLED_Clear();
	OLED_ShowString(1, 1, "Password");

	// 把触发键作为第1位密码
	if (firstKey != 0xFF && IsDigitKey(firstKey))
	{
		buf[pos] = KeyToDigit(firstKey);
		pos++;
	}

	while (pos < PASSWORD_LENGTH)
	{
		OLED_ShowString(2, 1, "    ");
		{
			unsigned char j;
			for (j = 0; j < pos; j++)
				OLED_ShowChar(2, 1+j, '*');
		}

		key = Key_Scan();
		if (key == 0xFF) continue;
		if (key == 12) { OLED_Clear(); return; }  // * 取消

		if (IsDigitKey(key))
		{
			buf[pos] = KeyToDigit(key);
			pos++;
		}
	}

	delay_10ms(30);

	// 比对密码
	if (memcmp(buf, AdminPassword, PASSWORD_LENGTH) == 0)
	{
		OLED_Clear();
		OLED_ShowString(1, 1, "Smart Lock");
		OLED_ShowString(2, 1, "True");
		LED_ON;
		delay_10ms(200);
		LED_OFF;
	}
	else
	{
		OLED_Clear();
		OLED_ShowString(1, 1, "Smart Lock");
		OLED_ShowString(2, 1, "False");
		LED_Blink(3, 5);
	}
	OLED_Clear();
}

void App_Init(void)
{
	OLED_ShowString(1, 1, "Smart Lock");
	OLED_ShowString(2, 1, "Ready");
}

void App_ProcessKey(void)
{
	unsigned char key = Key_Get();

	if (key == 14)                  // # 键
	{
		HoldTime++;
		if (HoldTime >= 150)
		{
			HoldTime = 0;
			while (Key_Get() == 14) delay_10ms(1);
			Admin_Enter();
			OLED_Clear();
			OLED_ShowString(1, 1, "Smart Lock");
			OLED_ShowString(2, 1, "Ready");
		}
	}
	else if (IsDigitKey(key))       // 数字键 → 直接作为第1位进入密码
	{
		HoldTime = 0;
		while (Key_Get() == key) delay_10ms(1);  // 等松手
		App_PasswordUnlock(key);                 // 传入触发的键值
		OLED_Clear();
		OLED_ShowString(1, 1, "Smart Lock");
		OLED_ShowString(2, 1, "Ready");
	}
	else
	{
		HoldTime = 0;
	}
}

void App_ProcessCard(unsigned char *snr)
{
	if (CheckCard(snr))
	{
		OLED_Clear();
		OLED_ShowString(1, 1, "Smart Lock");
		OLED_ShowString(2, 1, "True");
		LED_ON;
		WaitCardOff();
		OLED_Clear();
		OLED_ShowString(1, 1, "Smart Lock");
		OLED_ShowString(2, 1, "Ready");
	}
	else
	{
		OLED_Clear();
		OLED_ShowString(1, 1, "Smart Lock");
		OLED_ShowString(2, 1, "False");
		LED_Blink(3, 5);
		WaitCardOff();
		OLED_Clear();
		OLED_ShowString(1, 1, "Smart Lock");
		OLED_ShowString(2, 1, "Ready");
	}
	LED_OFF;
}