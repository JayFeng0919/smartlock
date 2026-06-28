#ifndef __LOCK_H
#define __LOCK_H

#include "stm32f10x.h"

/* ── 门锁硬件初始化 ───────────────────────────────
 * Lock_GPIO_Init: RC522 SPI 引脚 + LED 引脚 + BEEP引脚
 * Lock_RC522_Init: RC522 复位 → 配置 ISO14443A → 开天线
 * ──────────────────────────────────────────────── */

void Lock_GPIO_Init(void);
void Lock_RC522_Init(void);

/* ── 蜂鸣器辅助 ────────────────────────────────── */
void BEEP_Beep(unsigned char times, unsigned int duration);

#endif
