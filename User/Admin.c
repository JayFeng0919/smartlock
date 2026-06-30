#include "stm32f10x.h"
#include "OLED.h"
#include "main.h"
#include "mfrc522.h"
#include "Key.h"
#include "Admin.h"
#include <string.h>

/* ═══════════════════════════════════════════════════
 *  Admin.c — 管理员模式
 *
 *  两级菜单结构：
 *    密码验证 → 一级菜单(类别) → 二级菜单(子功能)
 *
 *  一级：1.Card  2.Finger  3.PW  *.Exit
 *  卡片二级：1.Add  2.Del  3.View  *.Back
 *  指纹二级：1.Add  2.ClearAll  3.Count  *.Back
 *  密码：直接进入改密流程
 * ═══════════════════════════════════════════════════ */

/* ── 全局变量 ──────────────────────────────────── */
unsigned char WhiteList[MAX_CARD_COUNT][4];
unsigned char CardCount = 0;
unsigned char AdminPassword[PASSWORD_LENGTH] = {0,0,0,0};

/* ── 内部辅助 ──────────────────────────────────── */

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

static void RemoveCard(unsigned char *snr)
{
	unsigned char i, j;
	for (i = 0; i < CardCount; i++)
	{
		if (memcmp(WhiteList[i], snr, 4) == 0)
		{
			for (j = i; j < CardCount - 1; j++)
				memcpy(WhiteList[j], WhiteList[j+1], 4);
			CardCount--;
			return;
		}
	}
}

static void ShowCardSNR(unsigned char line, unsigned char *snr)
{
	unsigned char i;
	for (i = 0; i < 4; i++)
		OLED_ShowHexNum(line, 1 + i*3, snr[i], 2);
}

/* ── KeyToDigit ────────────────────────────────── */
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

/* ── InputPassword ───────────────────────────────
 * 输入4位密码，返回 1=正确, 0=取消/错误
 * ──────────────────────────────────────────────── */
static unsigned char InputPassword(unsigned char *correct)
{
	unsigned char buf[PASSWORD_LENGTH];
	unsigned char pos = 0, key, j;

	OLED_Clear();
	OLED_ShowString(1, 1, "Password:");

	while (pos < PASSWORD_LENGTH)
	{
		OLED_ShowString(2, 1, "    ");
		for (j = 0; j < pos; j++)
			OLED_ShowChar(2, 1+j, '*');

		key = Key_Scan();
		if (key == 0xFF) continue;
		if (key == 12) return 0;                /* * 取消 */

		if (key <= 10 || key == 13)             /* 数字键 0~9 */
		{
			buf[pos] = KeyToDigit(key);
			pos++;
			OLED_ShowChar(2, pos, '*');
		}
	}

	delay_10ms(30);
	if (memcmp(buf, correct, PASSWORD_LENGTH) == 0)
	{
		OLED_ShowString(2, PASSWORD_LENGTH+2, " OK");
		delay_10ms(100);
		return 1;
	}
	else
	{
		OLED_ShowString(2, PASSWORD_LENGTH+2, "ERR");
		delay_10ms(100);
		return 0;
	}
}

/* ═══════════════════════════════════════════════════
 *  一级菜单：选择管理类别
 * ═══════════════════════════════════════════════════ */

/*
 * ShowMainMenu — 一级菜单
 *   1.Card  2.Finger  3.PW  *.Exit
 *   返回：1=卡, 2=指纹, 3=密码, 0=退出
 */
static unsigned char ShowMainMenu(void)
{
	OLED_Clear();
	OLED_ShowString(1, 1, "1.Card  2.Finger");
	OLED_ShowString(2, 1, "3.PW      *.Exit");

	while (1)
	{
		unsigned char key = Key_Scan();
		if (key == 0xFF) continue;
		if (key == 0)                 return 1;   /* 1 → Card */
		if (key == 1)                 return 2;   /* 2 → Finger */
		if (key == 2)                 return 3;   /* 3 → PW */
		if (key == 12)                return 0;   /* * → Exit */
	}
}

/* ═══════════════════════════════════════════════════
 *  卡片管理 二级菜单
 * ═══════════════════════════════════════════════════ */

static void Admin_CardMenu(void)
{
	OLED_Clear();
	OLED_ShowString(1, 1, "1.Add  2.Del");
	OLED_ShowString(2, 1, "3.View  *.Back");

	while (1)
	{
		unsigned char key = Key_Scan();
		if (key == 0xFF) continue;

		if (key == 0)                                           /* 1 → 录入卡片 */
		{
			char status;
			unsigned char TagType[2], snr[4];

			OLED_Clear();
			OLED_ShowString(1, 1, "Add Card");
			OLED_ShowString(2, 1, "Place card...");

			while (1)
			{
				if (Key_Scan() == 12) break;    /* * 取消 */

				status = PcdRequest(REQ_ALL, TagType);
				if (!status)
				{
					status = PcdAnticoll(snr);
					if (!status)
					{
						status = PcdSelect(snr);
						if (!status)
						{
							if (CheckCard(snr))
							{
								OLED_Clear();
								OLED_ShowString(1, 1, "Already Exist");
								LED_Blink(2, 5);
							}
							else if (CardCount < MAX_CARD_COUNT)
							{
								memcpy(WhiteList[CardCount], snr, 4);
								CardCount++;
								OLED_Clear();
								OLED_ShowString(1, 1, "Added!");
								ShowCardSNR(2, snr);
								LED_ON; BEEP_Beep(1, 20); delay_10ms(20); LED_OFF;
							}
							else
							{
								OLED_Clear();
								OLED_ShowString(1, 1, "List Full!");
								delay_10ms(200);
							}
							WaitCardOff();
							break;
						}
					}
				}
			}
			OLED_Clear();
			OLED_ShowString(1, 1, "1.Add  2.Del");
			OLED_ShowString(2, 1, "3.View  *.Back");
		}

		else if (key == 1)                                      /* 2 → 删除卡片 */
		{
			char status;
			unsigned char TagType[2], snr[4];

			OLED_Clear();
			OLED_ShowString(1, 1, "Del Card");
			OLED_ShowString(2, 1, "Place card...");

			while (1)
			{
				if (Key_Scan() == 12) break;

				status = PcdRequest(REQ_ALL, TagType);
				if (!status)
				{
					status = PcdAnticoll(snr);
					if (!status)
					{
						status = PcdSelect(snr);
						if (!status)
						{
							if (!CheckCard(snr))
							{
								OLED_Clear();
								OLED_ShowString(1, 1, "Not Found!");
								delay_10ms(100);
							}
							else
							{
								OLED_Clear();
								OLED_ShowString(1, 1, "Sure?");
								ShowCardSNR(2, snr);
								OLED_ShowString(3, 1, "#=Yes  *=No");
								while (1)
								{
									unsigned char k = Key_Scan();
									if (k == 0xFF) continue;
									if (k == 14)      /* # 确认 */
									{
										RemoveCard(snr);
										OLED_Clear();
										OLED_ShowString(1, 1, "Deleted!");
										LED_ON; BEEP_Beep(1, 20); delay_10ms(20); LED_OFF;
										break;
									}
									if (k == 12) break; /* * 取消 */
								}
							}
							WaitCardOff();
							break;
						}
					}
				}
			}
			OLED_Clear();
			OLED_ShowString(1, 1, "1.Add  2.Del");
			OLED_ShowString(2, 1, "3.View  *.Back");
		}

		else if (key == 2)                                      /* 3 → 查看全部 */
		{
			unsigned char idx = 0;

			if (CardCount == 0)
			{
				OLED_Clear();
				OLED_ShowString(1, 1, "No cards");
				delay_10ms(200);
			}
			else
			{
				while (1)
				{
					OLED_Clear();
					OLED_ShowNum(1, 1, idx+1, 1);
					OLED_ShowChar(1, 2, '/');
					OLED_ShowNum(1, 3, CardCount, 1);
					ShowCardSNR(2, WhiteList[idx]);
					delay_10ms(100);
					if (Key_Scan() == 12) break;
					if (idx < CardCount - 1) idx++; else idx = 0;
				}
			}
			OLED_Clear();
			OLED_ShowString(1, 1, "1.Add  2.Del");
			OLED_ShowString(2, 1, "3.View  *.Back");
		}

		else if (key == 12)    /* * → 返回上级 */
		{
			return;
		}
	}
}

/* ═══════════════════════════════════════════════════
 *  指纹管理 二级菜单
 * ═══════════════════════════════════════════════════ */

static void Admin_FingerMenu(void)
{
	OLED_Clear();
	OLED_ShowString(1, 1, "1.Add 2.ClearAll");
	OLED_ShowString(2, 1, "3.Count  *.Back");

	while (1)
	{
		unsigned char key = Key_Scan();
		if (key == 0xFF) continue;

		if (key == 0)           /* 1 → 添加指纹 */
		{
			OLED_Clear();
			Finger_Add();
			OLED_Clear();
			OLED_ShowString(1, 1, "1.Add 2.ClearAll");
			OLED_ShowString(2, 1, "3.Count  *.Back");
		}
		else if (key == 1)      /* 2 → 清空指纹库 */
		{
			OLED_Clear();
			OLED_ShowString(1, 1, "Clear All?");
			OLED_ShowString(2, 1, "1=Yes  *=No");
			while (1)
			{
				unsigned char k = Key_Scan();
				if (k == 0xFF) continue;
				if (k == 0)  { Finger_ClearAll(); break; }
				if (k == 12) break;
			}
			OLED_Clear();
			OLED_ShowString(1, 1, "1.Add 2.ClearAll");
			OLED_ShowString(2, 1, "3.Count  *.Back");
		}
		else if (key == 2)      /* 3 → 查看数量 */
		{
			unsigned char cnt = Finger_GetCount();
			OLED_Clear();
			OLED_ShowString(1, 1, "Finger Count:");
			OLED_ShowNum(2, 1, cnt, 3);
			delay_10ms(200);
			OLED_Clear();
			OLED_ShowString(1, 1, "1.Add 2.ClearAll");
			OLED_ShowString(2, 1, "3.Count  *.Back");
		}
		else if (key == 12)     /* * → 返回上级 */
		{
			return;
		}
	}
}

/* ═══════════════════════════════════════════════════
 *  密码修改
 * ═══════════════════════════════════════════════════ */

static void Admin_ChangePassword(void)
{
	unsigned char newBuf[PASSWORD_LENGTH];
	unsigned char confirmBuf[PASSWORD_LENGTH];
	unsigned char pos, key, j;

	/* 旧密码验证 */
	if (!InputPassword(AdminPassword))
		return;

	/* 输入新密码 */
	OLED_Clear();
	OLED_ShowString(1, 1, "New PW:");
	pos = 0;
	while (pos < PASSWORD_LENGTH)
	{
		OLED_ShowString(2, 1, "    ");
		for (j = 0; j < pos; j++)
			OLED_ShowChar(2, 1+j, '*');
		key = Key_Scan();
		if (key == 0xFF) continue;
		if (key == 12) return;
		if (key <= 10 || key == 13) { newBuf[pos] = KeyToDigit(key); pos++; }
	}
	delay_10ms(30);

	/* 确认新密码 */
	OLED_Clear();
	OLED_ShowString(1, 1, "Confirm PW:");
	pos = 0;
	while (pos < PASSWORD_LENGTH)
	{
		OLED_ShowString(2, 1, "    ");
		for (j = 0; j < pos; j++)
			OLED_ShowChar(2, 1+j, '*');
		key = Key_Scan();
		if (key == 0xFF) continue;
		if (key == 12) return;
		if (key <= 10 || key == 13) { confirmBuf[pos] = KeyToDigit(key); pos++; }
	}
	delay_10ms(30);

	if (memcmp(newBuf, confirmBuf, PASSWORD_LENGTH) == 0)
	{
		memcpy(AdminPassword, newBuf, PASSWORD_LENGTH);
		OLED_Clear();
		OLED_ShowString(1, 1, "Password");
		OLED_ShowString(2, 1, "Changed!");
		BEEP_Beep(2, 10);
		delay_10ms(200);
	}
	else
	{
		OLED_Clear();
		OLED_ShowString(1, 1, "Mismatch!");
		OLED_ShowString(2, 1, "Try again");
		delay_10ms(200);
	}
}

/* ═══════════════════════════════════════════════════
 *  外部接口
 * ═══════════════════════════════════════════════════ */

void Admin_Init(void)
{
	unsigned char i;
	for (i = 0; i < PASSWORD_LENGTH; i++)
		AdminPassword[i] = 0;
}

void Admin_Enter(void)
{
	unsigned char tryCount = 0;
	unsigned char ok;

	/* ── 密码验证（最多3次） ── */
	while (tryCount < 3)
	{
		ok = InputPassword(AdminPassword);
		if (ok) break;
		tryCount++;
		if (tryCount >= 3)
		{
			OLED_Clear();
			OLED_ShowString(1, 1, "Locked!");
			OLED_ShowString(2, 1, "Wait 10s...");
			BEEP_Beep(1, 50);
			delay_10ms(1000);
			return;
		}
	}

	/* ── 主循环：一级菜单 → 二级分发 ── */
	while (1)
	{
		unsigned char mainChoice = ShowMainMenu();
		switch (mainChoice)
		{
			case 0:                                      /* 退出 */
				return;
			case 1:  Admin_CardMenu();      break;       /* 卡片管理 */
			case 2:  Admin_FingerMenu();    break;       /* 指纹管理 */
			case 3:  Admin_ChangePassword(); break;      /* 修改密码 */
			default: break;
		}
	}
}