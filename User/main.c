#include "stm32f10x.h"
#include "main.h"

/* ═══════════════════════════════════════════════════
 *  main.c — 智能门锁 主程序
 *
 *  职责：系统初始化 + 主循环调度
 *  具体业务逻辑见 App.c（常态模式）和 Admin.c（管理员模式）
 * ═══════════════════════════════════════════════════ */

int main(void)
{
	char status;
	unsigned char TagType[2];
	unsigned char SelectedSnr[4];

	/* ═══ 模块初始化（顺序与参考工程一致，确保指纹模组正确上电）═══ */
	Timer_Init();                             // 0. TIM2 定时器
	delay_10ms(8);                            // 上电后延时（参考工程建议）
	Serial_Init();                            // 1. USART2（指纹模块通信）
	Finger_Init();                            // 2. 指纹模块 GPIO 初始化
	OLED_Init();                              // 3. OLED 显示屏
	Key_Init();                               // 4. 矩阵键盘
	Lock_GPIO_Init();                         // 5. RC522 + LED + BEEP 引脚
	Lock_RC522_Init();                        // 6. RC522 复位 + 配置
	Admin_Init();                             // 7. 加载管理员密码
	App_Init();                               // 8. 显示常态界面

	/* ═══ 主循环：10ms 周期 ═══ */
	while (1)
	{
		/* 按键处理：长按 # 1.5秒 → 管理员模式 */
		App_ProcessKey();

		/* 刷卡处理：ISO14443 标准三部曲 */
		status = PcdRequest(REQ_ALL, TagType);
		if (!status)
		{
			status = PcdAnticoll(SelectedSnr);
			if (!status)
			{
				status = PcdSelect(SelectedSnr);
				if (!status)
				{
					App_ProcessCard(SelectedSnr);
				}
			}
		}

		/* 指纹检测：手指放到模块上 → 指纹开锁 */
		if (FINGER_TOUCH_READ)
		{
			delay_10ms(5);              // 消抖
			if (FINGER_TOUCH_READ)
			{
				Finger_Unlock();
				OLED_Clear();
				App_Init();
			}
		}

		LED_OFF;
		delay_10ms(1);
	}
}