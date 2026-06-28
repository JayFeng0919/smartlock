#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

/* ── 4x4 矩阵键盘模块 ──
 *  行 PA8~11 (推挽输出)  列 PB15~12 (上拉输入)
 *  键值: row*4+col (0='1', 14='#', 15='D', 0xFF=无)
 * ──────────────────────── */

void Key_Init(void);               // 初始化 GPIO
unsigned char Key_Scan(void);      // 扫描按键（消抖+松手检测），返回键值，无按下返?? 0xFF
unsigned char Key_Get(void);       // 快速扫描一次（无消抖），返回键值，无按下返回 0xFF
char Key_ToChar(unsigned char k);  // 键值转 ASCII 字符

#endif