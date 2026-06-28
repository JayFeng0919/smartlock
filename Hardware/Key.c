#include "stm32f10x.h"
#include "main.h"
#include "mfrc522.h"
#include "Key.h"

/* ═══════════════════════════════════════════════════
 *  Key.c — 4x4 矩阵键盘驱动
 *
 *  扫描方式：逐行拉低，读列电平，消抖确认
 *  键值编码：row*4 + col（0='1', 15='D'）
 * ═══════════════════════════════════════════════════ */

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

/* ── 逐行扫描获取键值（无消抖，供内部调用） ──────── */
static unsigned char ScanRaw(void)
{
	// 行1
	KEY_ROW1_L;
	if      (KEY_COL1_READ == 0) { KEY_ROW1_H; return 0; }   // R1C1 = '1'
	else if (KEY_COL2_READ == 0) { KEY_ROW1_H; return 1; }   // R1C2 = '2'
	else if (KEY_COL3_READ == 0) { KEY_ROW1_H; return 2; }   // R1C3 = '3'
	else if (KEY_COL4_READ == 0) { KEY_ROW1_H; return 3; }   // R1C4 = 'A'
	KEY_ROW1_H;

	// 行2
	KEY_ROW2_L;
	if      (KEY_COL1_READ == 0) { KEY_ROW2_H; return 4; }   // R2C1 = '4'
	else if (KEY_COL2_READ == 0) { KEY_ROW2_H; return 5; }   // R2C2 = '5'
	else if (KEY_COL3_READ == 0) { KEY_ROW2_H; return 6; }   // R2C3 = '6'
	else if (KEY_COL4_READ == 0) { KEY_ROW2_H; return 7; }   // R2C4 = 'B'
	KEY_ROW2_H;

	// 行3
	KEY_ROW3_L;
	if      (KEY_COL1_READ == 0) { KEY_ROW3_H; return 8; }   // R3C1 = '7'
	else if (KEY_COL2_READ == 0) { KEY_ROW3_H; return 9; }   // R3C2 = '8'
	else if (KEY_COL3_READ == 0) { KEY_ROW3_H; return 10; }  // R3C3 = '9'
	else if (KEY_COL4_READ == 0) { KEY_ROW3_H; return 11; }  // R3C4 = 'C'
	KEY_ROW3_H;

	// 行4
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

	// 消抖：等 20ms 再扫一次
	delay_10ms(2);
	if (ScanRaw() != key)
		return 0xFF;

	// 等待按键释放
	while (ScanRaw() != 0xFF)
	{
		delay_10ms(1);
	}

	return key;
}


/* ── Key_Get: 快速扫描（无消抖无等待） ───────────── */
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