#include "stm32f10x.h"
#include "OLED.h"
#include "main.h"
#include "mfrc522.h"
#include "Key.h"
#include "Admin.h"
#include "App.h"
#include "QR.h"
#include <string.h>

/* ═══════════════════════════════════════════════════
 *  App.c — 常态模式
 *
 *  开锁：IC卡 / 4位密码
 *  管理：长按 # 1.5秒
 *  密码：按任意数字键 → 该键作为第1位 → 继续输后3位
 * ═══════════════════════════════════════════════════ */

#define MAX_PW_RETRY        3

extern unsigned char WhiteList[MAX_CARD_COUNT][4];
extern unsigned char CardCount;
extern unsigned char AdminPassword[PASSWORD_LENGTH];

static unsigned int HoldTime = 0;
static unsigned char QRLatched = 0;
static unsigned char QRIgnoreUntilClear = 0;
static unsigned char LastQRData[QR_MAX_PAYLOAD_LENGTH];
static unsigned char LastQRLength = 0;

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
		Lock_AccessGrant(200);
	}
	else
	{
		OLED_Clear();
		OLED_ShowString(1, 1, "Smart Lock");
		OLED_ShowString(2, 1, "False");
		Lock_AccessDeny();
	}
	OLED_Clear();
}

void App_Init(void)
{
	OLED_ShowString(1, 1, "Smart Lock");
	OLED_ShowString(2, 1, "Ready");
}

/* 常态界面按键检测 */
void App_ProcessKey(void)
{
	unsigned char key = Key_Get();

	if (key == 14)                  // # 键
	{
		HoldTime++;
		if (HoldTime >= 30)
		{
			HoldTime = 0;
			while (Key_Get() == 14) delay_10ms(1);
			Admin_Enter();
			/*
			 * 管理员录入二维码后，必须等当前二维码离开画面，
			 * 避免退出菜单时立刻用刚录入的二维码开锁。
			 */
			QR_FlushEvents();
			QRIgnoreUntilClear = 1;
			QRLatched = 1;
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

/* OpenMV二维码事件处理 */
void App_ProcessQR(void)
{
	unsigned char Data[QR_MAX_PAYLOAD_LENGTH];
	unsigned char Length = 0;
	unsigned char Event = QR_GetEvent(Data, &Length);

	if (Event == QR_EVENT_NONE)
		return;

	if (Event == QR_EVENT_CLEAR)
	{
		QRLatched = 0;
		QRIgnoreUntilClear = 0;
		LastQRLength = 0;
		return;
	}

	if (Event != QR_EVENT_DATA)
		return;
	if (QRIgnoreUntilClear)
		return;

	/* 相同二维码停留在画面中时只处理一次。 */
	if (QRLatched &&
	    (Length == LastQRLength) &&
	    (memcmp(Data, LastQRData, Length) == 0))
	{
		return;
	}

	memcpy(LastQRData, Data, Length);
	LastQRLength = Length;
	QRLatched = 1;

	OLED_Clear();
	OLED_ShowString(1, 1, "Smart Lock");

	if (QR_IsAuthorized(Data, Length))
	{
		OLED_ShowString(2, 1, "QR True");
		Lock_AccessGrant(100);
	}
	else
	{
		if (QR_HasAuthorized())
			OLED_ShowString(2, 1, "QR False");
		else
			OLED_ShowString(2, 1, "QR Not Set");
		Lock_AccessDeny();
	}

	OLED_Clear();
	App_Init();
}

/* 常态界面IC卡检测 */
void App_ProcessCard(unsigned char *snr)
{
	if (CheckCard(snr))
	{
		OLED_Clear();
		OLED_ShowString(1, 1, "Smart Lock");
		OLED_ShowString(2, 1, "True");
		Lock_AccessGrant(0);
		WaitCardOff();
		Lock_AccessClose();
		OLED_Clear();
		OLED_ShowString(1, 1, "Smart Lock");
		OLED_ShowString(2, 1, "Ready");
	}
	else
	{
		OLED_Clear();
		OLED_ShowString(1, 1, "Smart Lock");
		OLED_ShowString(2, 1, "False");
		Lock_AccessDeny();
		WaitCardOff();
		OLED_Clear();
		OLED_ShowString(1, 1, "Smart Lock");
		OLED_ShowString(2, 1, "Ready");
	}
	LED_OFF;
}
