#include "stm32f10x.h"
#include "main.h"       // 引脚宏定义（RST/MISO/MOSI/SCK/NSS/LED/BEEP/FINGER）
#include "mfrc522.h"    // RC522驱动函数
#include "Lock.h"

/* ── Lock_GPIO_Init ─────────────────────────────────────────────────────
 * 初始化所有 GPIO 引脚：
 *   RC522 SPI: RST(PB1) MISO(PA6) MOSI(PA7) SCK(PB10) NSS(PB11)
 *   LED: PC13 (推挽输出)
 *   BEEP: PB5 (推挽输出)
 *   指纹 PA4/PA0：由 Finger_Init() 独立初始化
 * ─────────────────────────────────────────────────────────────────────── */
void Lock_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	// ── RST 复位引脚 ──
	RCC_APB2PeriphClockCmd(MF522_RST_CLK, ENABLE);
	GPIO_InitStructure.GPIO_Pin = MF522_RST_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(MF522_RST_PORT, &GPIO_InitStructure);

	// ── MISO 主入从出（读取RC522返回数据） ──
	RCC_APB2PeriphClockCmd(MF522_MISO_CLK, ENABLE);
	GPIO_InitStructure.GPIO_Pin = MF522_MISO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(MF522_MISO_PORT, &GPIO_InitStructure);

	// ── MOSI 主出从入（向RC522发送数据） ──
	RCC_APB2PeriphClockCmd(MF522_MOSI_CLK, ENABLE);
	GPIO_InitStructure.GPIO_Pin = MF522_MOSI_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(MF522_MOSI_PORT, &GPIO_InitStructure);

	// ── SCK 时钟线 ──
	RCC_APB2PeriphClockCmd(MF522_SCK_CLK, ENABLE);
	GPIO_InitStructure.GPIO_Pin = MF522_SCK_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(MF522_SCK_PORT, &GPIO_InitStructure);

	// ── NSS 片选线 ──
	RCC_APB2PeriphClockCmd(MF522_NSS_CLK, ENABLE);
	GPIO_InitStructure.GPIO_Pin = MF522_NSS_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(MF522_NSS_PORT, &GPIO_InitStructure);

	// ── LED (PC13) ──
	RCC_APB2PeriphClockCmd(LED_CLK, ENABLE);
	GPIO_InitStructure.GPIO_Pin = LED_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(LED_PORT, &GPIO_InitStructure);

	// ── BEEP (PB5) ──
	RCC_APB2PeriphClockCmd(BEEP_CLK, ENABLE);
	GPIO_InitStructure.GPIO_Pin = BEEP_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(BEEP_PORT, &GPIO_InitStructure);
	BEEP_OFF;
}

/* ── Lock_RC522_Init ────────────────────────────────────────────────────
 * 初始化RC522模块：复位 → 关天线 → 开天线 → 配置ISO14443A
 * ─────────────────────────────────────────────────────────────────────── */
void Lock_RC522_Init(void)
{
	LED_OFF;
	delay_10ms(10);
	PcdReset();
	PcdAntennaOff();
	PcdAntennaOn();
	M500PcdConfigISOType('A');
}


/* ── LED_Blink ─────────────────────────────────────────────────────────
 * LED 闪烁辅助函数
 * times: 闪烁次数，duration: 每次亮/灭时长（x10ms）
 * ─────────────────────────────────────────────────────────────────────── */
void LED_Blink(unsigned char times, unsigned int duration)
{
	unsigned char i;
	for (i = 0; i < times; i++)
	{
		LED_ON;  delay_10ms(duration);
		LED_OFF; if (i < times - 1) delay_10ms(duration);
	}
}

/* ── BEEP_Beep ─────────────────────────────────────────────────────────
 * 蜂鸣器辅助函数
 * times: 鸣叫次数，duration: 每次时长（x10ms）
 * ─────────────────────────────────────────────────────────────────────── */
void BEEP_Beep(unsigned char times, unsigned int duration)
{
	unsigned char i;
	for (i = 0; i < times; i++)
	{
		BEEP_ON;  delay_10ms(duration);
		BEEP_OFF; if (i < times - 1) delay_10ms(duration);
	}
}