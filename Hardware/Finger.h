#ifndef __FINGER_H
#define __FINGER_H

#include "stm32f10x.h"

/* ── 指纹模块对外接口 ──────────────────────────── */
void  Finger_Init(void);                 // 初始化（GPIO）
void  Finger_Unlock(void);               // 指纹开锁（1:N识别验证）
void  Finger_Add(void);                  // 添加一枚指纹（自动分配ID）
void  Finger_ClearAll(void);             // 清空指纹库（删除全部）
unsigned char Finger_GetCount(void);     // 查询已录入数量

#endif