#include "stm32f10x.h"
#include "main.h"       // 引脚宏定义（RST/MISO/MOSI/SCK/NSS/LED/BEEP）
#include "mfrc522.h"    // RC522驱动函数
#include "Lock.h"

/* ── Lock_GPIO_Init ─────────────────────────────────────────────────────
 * 初始化与RC522通信所需的5个SPI模拟引脚 + LED指示灯引脚 +BEEP
 * SPI引脚分配（见 MAIN.H）：
 *   RST  → PB1  (推挽输出)   MISO → PA6  (浮空输入)
 *   MOSI → PA7  (推挽输出)   SCK  → PB10 (推挽输出)
 *   NSS  → PB11 (推挽输出)   LED  → PB0  (推挽输出)
*    BEEP → PB5  (推挽输出)
 * 注意：这里用的是软件模拟SPI，不是STM32硬件SPI
 * ─────────────────────────────────────────────────────────────────────── */
void Lock_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	// ── RST 复位引脚 ──
	RCC_APB2PeriphClockCmd(MF522_RST_CLK, ENABLE);
	GPIO_InitStructure.GPIO_Pin = MF522_RST_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;    // 推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(MF522_RST_PORT, &GPIO_InitStructure);

	// ── MISO 主入从出（读取RC522返回数据） ──
	RCC_APB2PeriphClockCmd(MF522_MISO_CLK, ENABLE);
	GPIO_InitStructure.GPIO_Pin = MF522_MISO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 浮空输入
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

	// ── NSS 片选线（低电平选中RC522） ──
	RCC_APB2PeriphClockCmd(MF522_NSS_CLK, ENABLE);
	GPIO_InitStructure.GPIO_Pin = MF522_NSS_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(MF522_NSS_PORT, &GPIO_InitStructure);

	// ── LED ──
	RCC_APB2PeriphClockCmd(LED_CLK, ENABLE);
	GPIO_InitStructure.GPIO_Pin = LED_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(LED_PORT, &GPIO_InitStructure);
	
	// ── BEEP ──
	RCC_APB2PeriphClockCmd(BEEP_CLK, ENABLE);
	GPIO_InitStructure.GPIO_Pin = BEEP_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(BEEP_PORT, &GPIO_InitStructure);

	BEEP_OFF;
}

/* ── Lock_RC522_Init ────────────────────────────────────────────────────
 * 初始化RC522模块的标准流程：
 *   1. 硬件复位（拉低→拉高）
 *   2. 关闭天线 → 重新开启（清空状态）
 *   3. 配置为 ISO14443 Type A 模式（Mifare S50/S70 卡）
 * ─────────────────────────────────────────────────────────────────────── */
/* ── RC522初始化顺序：复位→重置天线→配置协议 ── */
void Lock_RC522_Init(void)
{
	LED_OFF;
	delay_10ms(10);         // 延时让RC522稳定
	PcdReset();             // RC522软复位
	PcdAntennaOff();        // 先关天线
	PcdAntennaOn();         // 再开天线（确保状态干净）
	M500PcdConfigISOType('A'); // 配置为ISO14443A协议
}

/* ── BEEP_Beep ─────────────────────────────────────────────────────────
 * 蜂鸣器辅助函数
 * times: 鸣叫次数
 * duration: 每次鸣叫/间隔的时长（×10ms，即 duration=10 表示 100ms）
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
