#ifndef __LOCK_H
#define __LOCK_H

#include "stm32f10x.h"

/* ── 门锁硬件初始化 ───────────────────────────────
 * Lock_GPIO_Init: RC522 SPI + LED + BEEP + Finger 引脚初始化
 * Lock_RC522_Init: RC522 复位 → 配置 ISO14443A → 开天线
 * ──────────────────────────────────────────────── */

void Lock_GPIO_Init(void);
void Lock_RC522_Init(void);

/* ── 蜂鸣器辅助 ────────────────────────────────── */
void BEEP_Beep(unsigned char times, unsigned int duration);
void LED_Blink(unsigned char times, unsigned int duration);

#endif
