#include "stm32f10x.h"
#include "main.h"
#include "Key.h"

/* ── 4x4 矩阵键盘 ───────────────────────────────────
 *  R1 R2 R3 R4  → PA8 PA9 PA10 PA11（行，推挽输出）
 *  C1 C2 C3 C4  → PB12 PB13 PB14 PB15（列，上拉输入）
 *  布局：
 *     C1   C2   C3   C4
 *  R1  1    2    3    A
 *  R2  4    5    6    B
 *  R3  7    8    9    C
 *  R4  *    0    #    D
 * ──────────────────────────────────────────────────── */

static const char KeyMap[16] = {
	'1', '2', '3', 'A',
	'4', '5', '6', 'B',
	'7', '8', '9', 'C',
	'*', '0', '#', 'D'
};

void Key_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(KEY_ROW_CLK, ENABLE);
	GPIO_InitStructure.GPIO_Pin = KEY_ROW_PINS;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(KEY_ROW_PORT, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(KEY_COL_CLK, ENABLE);
	GPIO_InitStructure.GPIO_Pin = KEY_COL_PINS;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(KEY_COL_PORT, &GPIO_InitStructure);

	KEY_ROW1_H; KEY_ROW2_H; KEY_ROW3_H; KEY_ROW4_H;
}

/* 矩阵键盘读取（辅助函数无消抖） */
static unsigned char ScanRaw(void)
{
	// ROW1
	KEY_ROW1_L;
	if      (KEY_COL1_READ == 0) { KEY_ROW1_H; return 0; }   // R1C1 = '1'
	else if (KEY_COL2_READ == 0) { KEY_ROW1_H; return 1; }   // R1C2 = '2'
	else if (KEY_COL3_READ == 0) { KEY_ROW1_H; return 2; }   // R1C3 = '3'
	else if (KEY_COL4_READ == 0) { KEY_ROW1_H; return 3; }   // R1C4 = 'A'
	KEY_ROW1_H;

	// ROW2
	KEY_ROW2_L;
	if      (KEY_COL1_READ == 0) { KEY_ROW2_H; return 4; }   // R2C1 = '4'
	else if (KEY_COL2_READ == 0) { KEY_ROW2_H; return 5; }   // R2C2 = '5'
	else if (KEY_COL3_READ == 0) { KEY_ROW2_H; return 6; }   // R2C3 = '6'
	else if (KEY_COL4_READ == 0) { KEY_ROW2_H; return 7; }   // R2C4 = 'B'
	KEY_ROW2_H;

	// ROW3
	KEY_ROW3_L;
	if      (KEY_COL1_READ == 0) { KEY_ROW3_H; return 8; }   // R3C1 = '7'
	else if (KEY_COL2_READ == 0) { KEY_ROW3_H; return 9; }   // R3C2 = '8'
	else if (KEY_COL3_READ == 0) { KEY_ROW3_H; return 10; }  // R3C3 = '9'
	else if (KEY_COL4_READ == 0) { KEY_ROW3_H; return 11; }  // R3C4 = 'C'
	KEY_ROW3_H;

	// ROW4
	KEY_ROW4_L;
	if      (KEY_COL1_READ == 0) { KEY_ROW4_H; return 12; }  // R4C1 = '*'
	else if (KEY_COL2_READ == 0) { KEY_ROW4_H; return 13; }  // R4C2 = '0'
	else if (KEY_COL3_READ == 0) { KEY_ROW4_H; return 14; }  // R4C3 = '#'
	else if (KEY_COL4_READ == 0) { KEY_ROW4_H; return 15; }  // R4C4 = 'D'
	KEY_ROW4_H;

	return 0xFF;
}

unsigned char Key_Scan(void)
{
	unsigned char key;

	key = ScanRaw();
	if (key == 0xFF)
		return 0xFF;

	// 20ms消抖
	delay_10ms(2);
	if (ScanRaw() != key)
		return 0xFF;

	// 等待松手
	while (ScanRaw() != 0xFF)
	{
		delay_10ms(1);
	}

	return key;
}

unsigned char Key_Get(void)
{
	return ScanRaw();
}

char Key_ToChar(unsigned char k)
{
	if (k >= 16)
		return '\0';
	return KeyMap[k];
}