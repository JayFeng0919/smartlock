#ifndef __MAIN_H
#define __MAIN_H

#include "OLED.h"
#include "Lock.h"       // GPIO + RC522 初始化
#include "Key.h"        // 4x4 矩阵键盘
#include "mfrc522.h"    // RC522 寻卡/防冲突/选卡
#include "App.h"        // 常态模式（刷卡开锁 + 入口分流）
#include "Admin.h"      // 管理员模式（密码→菜单→增删查改）
#include "Delay.h"

/* ═══════════════════════════════════════════════════════════
 *  MAIN.H — 硬件引脚映射 & 宏定义
 *
 *  本文件定义了 STM32F103C8T6 与各外设的引脚连接关系
 *  修改引脚 / 换 MCU 时只需要改这里
 * ═══════════════════════════════════════════════════════════ */

/* ── 引脚总览 ────────────────────────────────────────
 *   PA0   → LED 指示灯（推挽输出）
 *   PA8~11 → 矩阵键盘 行 R1~R4（推挽输出）
 *   PB12~15→ 矩阵键盘 列 C1~C4（上拉输入）
 *   PB1   → RC522 RST
 *   PA6   → RC522 MISO（浮空输入）
 *   PA7   → RC522 MOSI
 *   PB10  → RC522 SCK
 *   PB11  → RC522 NSS
 *   PB8/9 → OLED I2C
 * ──────────────────────────────────────────────────── */

/* ── 4x4 矩阵键盘 ───────────────────────────────────
 *  R1 R2 R3 R4  → PA8 PA9 PA10 PA11（行，推挽输出）
 *  C1 C2 C3 C4  → PB15 PB14 PB13 PB12（列，上拉输入）
 *  布局：
 *     C1   C2   C3   C4
 *  R1  1    2    3    A
 *  R2  4    5    6    B
 *  R3  7    8    9    C
 *  R4  *    0    #    D
 * ──────────────────────────────────────────────────── */

#define KEY_ROW_PORT                     GPIOA
#define KEY_ROW_CLK                      RCC_APB2Periph_GPIOA
#define KEY_ROW1_PIN                     GPIO_Pin_8
#define KEY_ROW2_PIN                     GPIO_Pin_9
#define KEY_ROW3_PIN                     GPIO_Pin_10
#define KEY_ROW4_PIN                     GPIO_Pin_11
#define KEY_ROW_PINS                     (KEY_ROW1_PIN | KEY_ROW2_PIN | KEY_ROW3_PIN | KEY_ROW4_PIN)

#define KEY_COL_PORT                     GPIOB
#define KEY_COL_CLK                      RCC_APB2Periph_GPIOB
#define KEY_COL1_PIN                     GPIO_Pin_15
#define KEY_COL2_PIN                     GPIO_Pin_14
#define KEY_COL3_PIN                     GPIO_Pin_13
#define KEY_COL4_PIN                     GPIO_Pin_12
#define KEY_COL_PINS                     (KEY_COL1_PIN | KEY_COL2_PIN | KEY_COL3_PIN | KEY_COL4_PIN)

// 行操作宏
#define KEY_ROW1_H                       GPIO_SetBits(KEY_ROW_PORT, KEY_ROW1_PIN)
#define KEY_ROW1_L                       GPIO_ResetBits(KEY_ROW_PORT, KEY_ROW1_PIN)
#define KEY_ROW2_H                       GPIO_SetBits(KEY_ROW_PORT, KEY_ROW2_PIN)
#define KEY_ROW2_L                       GPIO_ResetBits(KEY_ROW_PORT, KEY_ROW2_PIN)
#define KEY_ROW3_H                       GPIO_SetBits(KEY_ROW_PORT, KEY_ROW3_PIN)
#define KEY_ROW3_L                       GPIO_ResetBits(KEY_ROW_PORT, KEY_ROW3_PIN)
#define KEY_ROW4_H                       GPIO_SetBits(KEY_ROW_PORT, KEY_ROW4_PIN)
#define KEY_ROW4_L                       GPIO_ResetBits(KEY_ROW_PORT, KEY_ROW4_PIN)

// 列读取宏
#define KEY_COL1_READ                    GPIO_ReadInputDataBit(KEY_COL_PORT, KEY_COL1_PIN)
#define KEY_COL2_READ                    GPIO_ReadInputDataBit(KEY_COL_PORT, KEY_COL2_PIN)
#define KEY_COL3_READ                    GPIO_ReadInputDataBit(KEY_COL_PORT, KEY_COL3_PIN)
#define KEY_COL4_READ                    GPIO_ReadInputDataBit(KEY_COL_PORT, KEY_COL4_PIN)

/* ── RC522 SPI 引脚映射 ────────────────────────────
 *     STM32          RC522
 *   ────────       ────────
 *   PB1  (RST) ─── RST
 *   PA6  (MISO) ── MISO    (MCU读取RC522数据)
 *   PA7  (MOSI) ── MOSI    (MCU向RC522写数据)
 *   PB10 (SCK) ─── SCK     (软件模拟SPI时钟)
 *   PB11 (NSS) ─── SDA/NSS (片选，低电平选中RC522)
 * ──────────────────────────────────────────────────── */

#define MF522_RST_PIN                    GPIO_Pin_1
#define MF522_RST_PORT                   GPIOB
#define MF522_RST_CLK                    RCC_APB2Periph_GPIOB

#define MF522_MISO_PIN                   GPIO_Pin_6
#define MF522_MISO_PORT                  GPIOA
#define MF522_MISO_CLK                   RCC_APB2Periph_GPIOA

#define MF522_MOSI_PIN                   GPIO_Pin_7
#define MF522_MOSI_PORT                  GPIOA
#define MF522_MOSI_CLK                   RCC_APB2Periph_GPIOA

#define MF522_SCK_PIN                    GPIO_Pin_10
#define MF522_SCK_PORT                   GPIOB
#define MF522_SCK_CLK                    RCC_APB2Periph_GPIOB

#define MF522_NSS_PIN                    GPIO_Pin_11
#define MF522_NSS_PORT                   GPIOB
#define MF522_NSS_CLK                    RCC_APB2Periph_GPIOB

#define LED_PIN                          GPIO_Pin_0
#define LED_PORT                         GPIOA
#define LED_CLK                          RCC_APB2Periph_GPIOA

/* ── RC522 引脚快捷操作宏 ────────────────────────── */
#define RST_H                            GPIO_SetBits(MF522_RST_PORT, MF522_RST_PIN)
#define RST_L                            GPIO_ResetBits(MF522_RST_PORT, MF522_RST_PIN)
#define MOSI_H                           GPIO_SetBits(MF522_MOSI_PORT, MF522_MOSI_PIN)
#define MOSI_L                           GPIO_ResetBits(MF522_MOSI_PORT, MF522_MOSI_PIN)
#define SCK_H                            GPIO_SetBits(MF522_SCK_PORT, MF522_SCK_PIN)
#define SCK_L                            GPIO_ResetBits(MF522_SCK_PORT, MF522_SCK_PIN)
#define NSS_H                            GPIO_SetBits(MF522_NSS_PORT, MF522_NSS_PIN)
#define NSS_L                            GPIO_ResetBits(MF522_NSS_PORT, MF522_NSS_PIN)
#define READ_MISO                        GPIO_ReadInputDataBit(MF522_MISO_PORT, MF522_MISO_PIN)

/* ── LED 宏 ──────────────────────────────────────── */
#define LED_ON                           GPIO_SetBits(LED_PORT, LED_PIN)
#define LED_OFF                          GPIO_ResetBits(LED_PORT, LED_PIN)

#define TRUE 1
#define FALSE 0

#endif