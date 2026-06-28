#include "stm32f10x.h"
#include "OLED.h"
#include "main.h"
#include "mfrc522.h"
#include "Key.h"
#include "Admin.h"
#include <string.h>

/* �T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T
 *  Admin.c �� ����Աģʽʵ��
 *
 *  ״̬������������ �� �˵�ѡ�� �� �ӹ��� �� ���ز˵�
 * �T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T */

/* ���� ȫ�ֱ��� ������������������������������������������������������������������������ */
unsigned char WhiteList[MAX_CARD_COUNT][4]; // ������
unsigned char CardCount = 0;                 // ��¼������
unsigned char AdminPassword[PASSWORD_LENGTH] = {0,0,0,0}; // ����Ա����(��������)

/* ���� �ڲ����� ������������������������������������������������������������������������ */

static void LED_Blink(unsigned char times, unsigned int dly)
{
	unsigned char i;
	for (i = 0; i < times; i++)
	{
		LED_ON;  delay_10ms(dly);
		LED_OFF; if (i < times - 1) delay_10ms(dly);
	}
}

// �ȶԿ���
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

/* ���� ɾ��ָ������ ���� ����ƥ�����λѹ������ ���� */
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

// ��ʾһ�о�������
static void ShowLine(unsigned char line, const char *str)
{
	OLED_ShowString(line, 1, (char *)str);
}

/* ���� ��ʾ4�ֽ�HEX���� ���� line:�� col��1��ʼ ���� */
static void ShowCardSNR(unsigned char line, unsigned char *snr)
{
	unsigned char i;
	for (i = 0; i < 4; i++)
	{
		OLED_ShowHexNum(line, 1 + i*3, snr[i], 2);
	}
}

/* ���� �������� ������������������������������������������������������������������������
 * OLED��ʾ "Password:" + ����λ��(��*����)
 * ���أ�1=������ȷ, 0=ȡ��/����
 * ������������������������������������������������������������������������������������������������ */
static unsigned char InputPassword(unsigned char *correct)
{
	unsigned char buf[PASSWORD_LENGTH];
	unsigned char pos = 0;
	unsigned char key;

	OLED_Clear();
	OLED_ShowString(1, 1, "Password:");

	while (pos < PASSWORD_LENGTH)
	{
		OLED_ShowString(2, 1, "    ");     // ���ǰһ������
		OLED_ShowString(2, 1, "    ");
		// ��ʾ������� *
		{
			unsigned char j;
			for (j = 0; j < pos; j++)
				OLED_ShowChar(2, 1+j, '*');
		}

		key = Key_Scan();
		if (key == 0xFF) continue;

		if (key == 12)  // * ����ȡ��
			return 0;

		if (key <= 10 || key == 13)    // ���ּ� 0~9
		{
			if (key == 13) buf[pos]=0; else if (key==0) buf[pos]=1; else if (key==1) buf[pos]=2; else if (key==2) buf[pos]=3; else if (key==4) buf[pos]=4; else if (key==5) buf[pos]=5; else if (key==6) buf[pos]=6; else if (key==8) buf[pos]=7; else if (key==9) buf[pos]=8; else if (key==10) buf[pos]=9; else continue;
			pos++;
			// ���� *
			OLED_ShowChar(2, pos, '*');
		}
	}

	// ȷ������
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

/* ���� �˵���ʾ ������������������������������������������������������������������������
 * ��1�У��˵�����
 * ��2�У���ǰѡ������¼���ҳ��
 * ������������������������������������������������������������������������������������������������ */
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
	OLED_ShowString(2, 1, "3.View 4.PW");

	// �ȴ�����
	while (1)
	{
		unsigned char key = Key_Scan();
		if (key == 0xFF) continue;

		if (key == 0) { OLED_Clear(); OLED_ShowString(1, 1, (char*)menu[0]); return 1; }   // 1
		if (key == 1) { OLED_Clear(); OLED_ShowString(1, 1, (char*)menu[1]); return 2; }   // 2
		if (key == 2) { OLED_Clear(); OLED_ShowString(1, 1, (char*)menu[2]); return 3; }   // 3
		if (key == 4) { OLED_Clear(); OLED_ShowString(1, 1, (char*)menu[3]); return 4; }   // 4
		if (key == 12) { return 0; }    // '*': �˳�
	}
}

/* ���� �ӹ���1��¼��IC�� �������������������������������������������������������� */
static void Admin_AddCard(void)
{
	char status;
	unsigned char TagType[2], snr[4];

	OLED_Clear();
	OLED_ShowString(1, 1, "Add Card");
	OLED_ShowString(2, 1, "Place card...");

	while (1)
	{
		if (Key_Scan() == 11) return;   // * ȡ��

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

/* ���� �ӹ���2��ɾ��IC�� �������������������������������������������������������� */
static void Admin_DelCard(void)
{
	char status;
	unsigned char TagType[2], snr[4];

	OLED_Clear();
	OLED_ShowString(1, 1, "Del Card");
	OLED_ShowString(2, 1, "Place card...");

	while (1)
	{
		if (Key_Scan() == 11) return;   // * ȡ��

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
						// ȷ��ɾ��
						OLED_Clear();
						OLED_ShowString(1, 1, "Sure?");
						OLED_ShowString(2, 1, "#=Yes *=No");

						while (1)
						{
							unsigned char key = Key_Scan();
							if (key == 14)      // # ȷ��
							{
								RemoveCard(snr);
								OLED_Clear();
								OLED_ShowString(1, 1, "Deleted!");
								BEEP_Beep(1, 20);               // 蜂鸣1声(200ms)表示删卡成功
								LED_Blink(2, 5);
								WaitCardOff();
								return;
							}
							if (key == 12)      // * ȡ��
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

/* ���� �ӹ���3���鿴��Ƭ�б� ������������������������������������������������ */
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
		if (Key_Scan() == 12) return;   // * ����
		if (idx < CardCount - 1)
			idx++;
		else
			idx = 0;
	}
}

/* ���� �ӹ���4���޸����� �������������������������������������������������������� */
static void Admin_ChangePassword(void)
{
	unsigned char oldBuf[PASSWORD_LENGTH];
	unsigned char newBuf[PASSWORD_LENGTH];
	unsigned char confirmBuf[PASSWORD_LENGTH];
	unsigned char pos, key;

	/* ��������� */
	if (!InputPassword(AdminPassword))
		return;

	/* ���������� */
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
		if (key == 12) return;          // * ȡ��
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

	/* ȷ�������� */
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
		if (key == 12) return;          // * ȡ��
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

/* ���� �ⲿ�ӿ� ������������������������������������������������������������������������ */

void Admin_Init(void)
{
	unsigned char i;
	// ����Ĭ������ "0000"
	for (i = 0; i < PASSWORD_LENGTH; i++)
		AdminPassword[i] = 0;
}

void Admin_Enter(void)
{
	unsigned char choice;
	unsigned char tryCount = 0;
	unsigned char ok;

	/* ���� ������֤�����3�Σ� ���� */
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
			delay_10ms(1000);      // ����10��;      // ��10��
			return;
		}
	}

	/* ���� ���˵�ѭ�� ���� */
	while (1)
	{
		choice = ShowMenu();
		switch (choice)
		{
			case 0:  return;                // �˳�����Աģʽ
			case 1:  Admin_AddCard();        break;
			case 2:  Admin_DelCard();        break;
			case 3:  Admin_ViewCards();      break;
			case 4:  Admin_ChangePassword(); break;
			default: break;
		}
	}
}