#ifndef __ADMIN_H
#define __ADMIN_H

#include "stm32f10x.h"

#define MAX_CARD_COUNT      5
#define PASSWORD_LENGTH     4

extern unsigned char AdminPassword[PASSWORD_LENGTH];  // 管理员/开锁密码

void Admin_Init(void);
void Admin_Enter(void);

#endif