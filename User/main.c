#include "stm32f10x.h"
#include "main.h"

/* ═══════════════════════════════════════════════════════════
 *  main.c — 智能门锁 主程序
 *
 *  职责：系统初始化 + 主循环调度
 *  具体业务逻辑见 App.c（常态模式）和 Admin.c（管理员模式）
 * ═══════════════════════════════════════════════════════════ */

int main(void)
{
	char status;                              // RC522 操作返回值
	unsigned char TagType[2];                 // 卡片类型（2字节）
	unsigned char SelectedSnr[4];             // 刷卡获取的4字节序列号

	/* ── 模块初始化（顺序敏感） ── */
	Lock_GPIO_Init();                         // 1. SPI引脚 + LED 先初始化
	Key_Init();                               // 2. 矩阵键盘 GPIO
	OLED_Init();                              // 3. OLED 显示屏
	Lock_RC522_Init();                        // 4. RC522 复位 + ISO14443A 配置
	Admin_Init();                             // 5. 加载管理员密码（默认0000）
	App_Init();                               // 6. 显示常态界面 Smart Lock / Ready

	/* ── 主循环：10ms 周期 ── */
	while (1)
	{
		/* 按键处理：长按 # 1.5秒 → 管理员模式 */
		App_ProcessKey();

		/* 刷卡处理：ISO14443 标准三部曲 */
		status = PcdRequest(REQ_ALL, TagType);      // 寻卡
		if (!status)                                // 找到卡
		{
			status = PcdAnticoll(SelectedSnr);       // 防冲突，拿序列号
			if (!status)
			{
				status = PcdSelect(SelectedSnr);     // 选中该卡
				if (!status)
				{
					App_ProcessCard(SelectedSnr);    // 比对白名单，显示结果
				}
			}
		}

		LED_OFF;                                    // 确保 LED 默认熄灭
		delay_10ms(1);                              // 循环节拍 ~10ms
	}
}