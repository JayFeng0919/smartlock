#ifndef __QR_H
#define __QR_H

#include "stm32f10x.h"

#define QR_I2C_ADDRESS          0x12U
#define QR_MAX_PAYLOAD_LENGTH   64U

#define QR_EVENT_NONE           0x00U
#define QR_EVENT_DATA           0x01U
#define QR_EVENT_CLEAR          0x02U

/* 便于在Keil Watch窗口观察通信状态。 */
extern volatile uint32_t QR_ValidFrameCount;
extern volatile uint32_t QR_CRCErrorCount;
extern volatile uint32_t QR_I2CErrorCount;

void QR_Init(void);

/*
 * 读取最新事件。
 * 返回QR_EVENT_DATA时，Data和Length中包含二维码原文；
 * 返回QR_EVENT_CLEAR时，表示二维码已离开画面。
 */
uint8_t QR_GetEvent(uint8_t *Data, uint8_t *Length);
void QR_FlushEvents(void);

uint8_t QR_HasAuthorized(void);
uint8_t QR_GetAuthorizedLength(void);
uint8_t QR_IsAuthorized(const uint8_t *Data, uint8_t Length);
uint8_t QR_SaveAuthorized(const uint8_t *Data, uint8_t Length);
uint8_t QR_ClearAuthorized(void);

#endif
