#include "stm32f10x.h"
#include "OLED.h"
#include "main.h"
#include "mfrc522.h"
#include "Key.h"
#include "Admin.h"
#include <string.h>

/* ==================================================
 *  Admin.c -- 管理员模式实现
 *
 *  状态机：密码输入 -> 菜单选择 -> 子功能 -> 返回菜单
 * ================================================== */

/* ---- 全局变量 ---- */
unsigned char WhiteList[MAX_CARD_COUNT][4]; // 白名单
unsigned char CardCount = 0;                  // 已录入数量
unsigned char AdminPassword[PASSWORD_LENGTH] = {0,0,0,0}; // 管理员密码(数字数组)

/* ---- 内部辅助 ---- */

// 比对卡号
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

/* ---- 删除指定卡号：遍历匹配后移位压缩数组 ---- */
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

// 显示一行居中文字
static void ShowLine(unsigned char line, const char *str)
{
	OLED_ShowString(line, 1, (char *)str);
}

/* ---- 显示4字节HEX卡号，line:行 col从1开始 ---- */
static void ShowCardSNR(unsigned char line, unsigned char *snr)
{
	unsigned char i;
	for (i = 0; i < 4; i++)
	{
		OLED_ShowHexNum(line, 1 + i*3, snr[i], 2);
	}
}

/* ---- 密码输入 ----
 * OLED显示 "Password:" + 已输位数(用*屏蔽)
 * 返回：1=密码正确, 0=取消/错误
 * ---------------------------------------------- */
static unsigned char InputPassword(unsigned char *correct)
{
	unsigned char buf[PASSWORD_LENGTH];
	unsigned char pos = 0;
	unsigned char key;

	OLED_Clear();
	OLED_ShowString(1, 1, "Password:");

	while (pos < PASSWORD_LENGTH)
	{
		OLED_ShowString(2, 1, "    ");     // 清空前一次输入
		OLED_ShowString(2, 1, "    ");
		// 显示已输入的 *
		{
			unsigned char j;
			for (j = 0; j < pos; j++)
				OLED_ShowChar(2, 1+j, '*');
		}

		key = Key_Scan();
		if (key == 0xFF) continue;

		if (key == 12)  // * 键：取消
			return 0;

		if (key <= 10 || key == 13)    // 数字键 0~9
		{
			if (key == 13) buf[pos]=0; else if (key==0) buf[pos]=1; else if (key==1) buf[pos]=2; else if (key==2) buf[pos]=3; else if (key==4) buf[pos]=4; else if (key==5) buf[pos]=5; else if (key==6) buf[pos]=6; else if (key==8) buf[pos]=7; else if (key==9) buf[pos]=8; else if (key==10) buf[pos]=9; else continue;
			pos++;
			// 回显 *
			OLED_ShowChar(2, pos, '*');
		}
	}

	// 确认密码
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

/* ---- 菜单显示 ----
 * 第1行：菜单标题
 * 第2行：当前选项
 * ---------------------------------------------- */
static unsigned char ShowMenu(void)
{
	const char *menu[] = {
		"1.Add Card",
		"2.Del Card",
		"3.View All",
		"4.ChangePW",
		"*.Exit"
	};

	OLED_Clear();
	OLED_ShowString(1, 1, "1.Add  2.Del");
	OLED_ShowString(2, 1, "3.View 4.PW 5.FP");

	// 等待按键
	while (1)
	{
		unsigned char key = Key_Scan();
		if (key == 0xFF) continue;

		if (key == 0) { OLED_Clear(); OLED_ShowString(1, 1, (char*)menu[0]); return 1; }   // 1
		if (key == 1) { OLED_Clear(); OLED_ShowString(1, 1, (char*)menu[1]); return 2; }   // 2
		if (key == 2) { OLED_Clear(); OLED_ShowString(1, 1, (char*)menu[2]); return 3; }   // 3
		if (key == 4) { OLED_Clear(); OLED_ShowString(1, 1, (char*)menu[3]); return 4; }   // 4
		if (key == 5) { OLED_Clear(); OLED_ShowString(1, 1, "5.Finger Print");  return 5; }   // 5
		if (key == 12) { return 0; }                                                        // * 退出
	}
}

/* ---- 子功能1：录入卡片 ---- */
static void Admin_AddCard(void)
{
	char status;
	unsigned char TagType[2], snr[4];

	OLED_Clear();
	OLED_ShowString(1, 1, "Add Card");
	OLED_ShowString(2, 1, "Place card...");

	while (1)
	{
		if (Key_Scan() == 12) return;   // * 取消

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
						BEEP_Beep(1, 20);                   // 蜂鸣1声(200ms)表示录卡成功
						LED_ON; delay_10ms(50); LED_OFF;
					}
					else
					{
						OLED_Clear();
						OLED_ShowString(1, 1, "List Full!");
						delay_10ms(200);
					}
					WaitCardOff();
					return;
				}
			}
		}
		delay_10ms(1);
	}
}

/* ---- 子功能2：删除卡片 ---- */
static void Admin_DelCard(void)
{
	char status;
	unsigned char TagType[2], snr[4];

	OLED_Clear();
	OLED_ShowString(1, 1, "Del Card");
	OLED_ShowString(2, 1, "Place card...");

	while (1)
	{
		if (Key_Scan() == 12) return;   // * 取消

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
						OLED_ShowString(1, 1, "Not in list");
						delay_10ms(100);
					}
					else
					{
					// 确认删除
						OLED_Clear();
						OLED_ShowString(1, 1, "Sure?");
						OLED_ShowString(2, 1, "#=Yes *=No");

						while (1)
						{
							unsigned char key = Key_Scan();
							if (key == 14)      // # 确认
							{
								RemoveCard(snr);
								OLED_Clear();
								OLED_ShowString(1, 1, "Deleted!");
								BEEP_Beep(1, 20);               // 蜂鸣1声(200ms)表示删卡成功
								LED_Blink(2, 5);
								WaitCardOff();
								return;
							}
							if (key == 12)      // * 取消
							{
								WaitCardOff();
								return;
							}
						}
					}
					WaitCardOff();
					return;
				}
			}
		}
		delay_10ms(1);
	}
}

/* ---- 子功能3：查看卡号白名单 ---- */
static void Admin_ViewCards(void)
{
	unsigned char idx = 0;

	if (CardCount == 0)
	{
		OLED_Clear();
		OLED_ShowString(1, 1, "No cards");
		delay_10ms(200);
		return;
	}

	while (1)
	{
		OLED_Clear();
		OLED_ShowNum(1, 1, idx+1, 1);
		OLED_ShowChar(1, 2, '/');
		OLED_ShowNum(1, 3, CardCount, 1);
		ShowCardSNR(2, WhiteList[idx]);
		delay_10ms(100);
		if (Key_Scan() == 12) return;   // * 返回
		if (idx < CardCount - 1)
			idx++;
		else
			idx = 0;
	}
}

/* ---- 子功能4：修改密码 ---- */
static void Admin_ChangePassword(void)
{
	unsigned char oldBuf[PASSWORD_LENGTH];
	unsigned char newBuf[PASSWORD_LENGTH];
	unsigned char confirmBuf[PASSWORD_LENGTH];
	unsigned char pos, key;

	/* 输入旧密码 */
	if (!InputPassword(AdminPassword))
		return;

	/* 输入新密码 */
	OLED_Clear();
	OLED_ShowString(1, 1, "New PW:");
	pos = 0;
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
		if (key == 12) return;          // * 取消
					if (key <= 10 || key == 13)
			{
				if (key == 13)      newBuf[pos] = 0;
				else if (key == 0)  newBuf[pos] = 1;
				else if (key == 1)  newBuf[pos] = 2;
				else if (key == 2)  newBuf[pos] = 3;
				else if (key == 4)  newBuf[pos] = 4;
				else if (key == 5)  newBuf[pos] = 5;
				else if (key == 6)  newBuf[pos] = 6;
				else if (key == 8)  newBuf[pos] = 7;
				else if (key == 9)  newBuf[pos] = 8;
				else if (key == 10) newBuf[pos] = 9;
				else                continue;
				pos++;
		}
	}

	delay_10ms(30);

	/* 确认新密码 */
	OLED_Clear();
	OLED_ShowString(1, 1, "Confirm PW:");
	pos = 0;
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
		if (key == 12) return;          // * 取消
		if (key <= 10 || key == 13)
		{
			if (key == 13)      confirmBuf[pos] = 0;
			else if (key == 0)  confirmBuf[pos] = 1;
			else if (key == 1)  confirmBuf[pos] = 2;
			else if (key == 2)  confirmBuf[pos] = 3;
			else if (key == 4)  confirmBuf[pos] = 4;
			else if (key == 5)  confirmBuf[pos] = 5;
			else if (key == 6)  confirmBuf[pos] = 6;
			else if (key == 8)  confirmBuf[pos] = 7;
			else if (key == 9)  confirmBuf[pos] = 8;
			else if (key == 10) confirmBuf[pos] = 9;
			else                continue;
			pos++;
		}
	}

	delay_10ms(30);

	if (memcmp(newBuf, confirmBuf, PASSWORD_LENGTH) == 0)
	{
		memcpy(AdminPassword, newBuf, PASSWORD_LENGTH);
		OLED_Clear();
		OLED_ShowString(1, 1, "Password");
		OLED_ShowString(2, 1, "Changed!");
		BEEP_Beep(2, 10);               // 蜂鸣2声(各100ms)表示改密成功
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

/* ---- 子功能5：指纹管理 ---- */
static void Admin_FingerMenu(void)
{
	OLED_Clear();
	OLED_ShowString(1, 1, "1.Add 2.ClearAll");
	OLED_ShowString(2, 1, "3.Count  *.Back");

	while (1)
	{
		unsigned char key = Key_Scan();
		if (key == 0xFF) continue;

		if (key == 0)      /* 1 → 添加指纹 */
		{
			OLED_Clear();
			Finger_Add();
			OLED_Clear();
			OLED_ShowString(1, 1, "1.Add 2.ClearAll");
			OLED_ShowString(2, 1, "3.Count  *.Back");
		}
		else if (key == 1) /* 2 → 清空指纹库 */
		{
			OLED_Clear();
			OLED_ShowString(1, 1, "Clear All?");
			OLED_ShowString(2, 1, "1=Yes  *=No");
			while (1)
			{
				unsigned char k = Key_Scan();
				if (k == 0xFF) continue;
				if (k == 0)      { Finger_ClearAll(); break; }  /* 1=确认 */
				if (k == 12)     break;                          /* *=取消 */
			}
			OLED_Clear();
			OLED_ShowString(1, 1, "1.Add 2.ClearAll");
			OLED_ShowString(2, 1, "3.Count  *.Back");
		}
		else if (key == 2) /* 3 → 查看数量 */
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
		else if (key == 12) /* * → 返回 */
		{
			return;
		}
	}
}
/* ---- 外部接口 ---- */

void Admin_Init(void)
{
	unsigned char i;
	// 加载默认密码 "0000"
	for (i = 0; i < PASSWORD_LENGTH; i++)
		AdminPassword[i] = 0;
}

void Admin_Enter(void)
{
	unsigned char choice;
	unsigned char tryCount = 0;
	unsigned char ok;

	/* ---- 密码验证（最多3次） ---- */
	while (tryCount < 3)
	{
		ok = InputPassword(AdminPassword);
		if (ok)
			break;
		tryCount++;
		if (tryCount >= 3)
		{
			OLED_Clear();
			OLED_ShowString(1, 1, "Locked!");
			OLED_ShowString(2, 1, "Wait 10s...");
			BEEP_Beep(1, 50);                       // 长鸣1声(500ms)警告
			delay_10ms(1000);      // 锁定10秒
			return;
		}
	}

	/* ---- 主菜单循环 ---- */
	while (1)
	{
		choice = ShowMenu();
		switch (choice)
		{
			case 0:  return;                // 退出管理员模式
			case 1:  Admin_AddCard();        break;
			case 2:  Admin_DelCard();        break;
			case 3:  Admin_ViewCards();      break;
			case 4:  Admin_ChangePassword(); break;
			case 5:  Admin_FingerMenu();     break;
			default: break;
		}
	}
}