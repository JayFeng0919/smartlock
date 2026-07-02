#include "QR.h"
#include <string.h>

/* OpenMV发送帧：A5 版本 类型 序号 长度 数据 CRC16_L CRC16_H */
#define QR_FRAME_HEADER          0xA5U
#define QR_PROTOCOL_VERSION      0x01U
#define QR_MSG_DATA              0x01U
#define QR_MSG_CLEAR             0x02U
#define QR_FRAME_OVERHEAD        7U
#define QR_RX_BUFFER_LENGTH      (QR_MAX_PAYLOAD_LENGTH + QR_FRAME_OVERHEAD)

/*
 * STM32F103C8官方Flash为64KB、页大小1KB。
 * 最后两个页专门保存配置，Keil工程的IROM终点必须限制在0x0800F800。
 */
#define QR_FLASH_PAGE_A          0x0800F800UL
#define QR_FLASH_PAGE_B          0x0800FC00UL
#define QR_FLASH_MAGIC           0x51524C4BUL
#define QR_FLASH_VERSION         0x0001U
#define QR_FLASH_RECORD_SIZE     80U
#define QR_FLASH_CRC_OFFSET      76U

static volatile uint8_t QR_RxFrame[QR_RX_BUFFER_LENGTH];
static volatile uint8_t QR_RxIndex;
static volatile uint8_t QR_PendingEvent;
static volatile uint8_t QR_PendingLength;
static volatile uint8_t QR_PendingData[QR_MAX_PAYLOAD_LENGTH];

static uint8_t QR_AuthorizedData[QR_MAX_PAYLOAD_LENGTH];
static uint8_t QR_AuthorizedLength;
static uint32_t QR_StorageSequence;
static uint32_t QR_ActiveFlashPage;

volatile uint32_t QR_ValidFrameCount;
volatile uint32_t QR_CRCErrorCount;
volatile uint32_t QR_I2CErrorCount;

static uint16_t QR_ReadLE16(const uint8_t *Data)
{
	return (uint16_t)Data[0] | ((uint16_t)Data[1] << 8);
}

static uint32_t QR_ReadLE32(const uint8_t *Data)
{
	return (uint32_t)Data[0] |
	       ((uint32_t)Data[1] << 8) |
	       ((uint32_t)Data[2] << 16) |
	       ((uint32_t)Data[3] << 24);
}

static void QR_WriteLE16(uint8_t *Data, uint16_t Value)
{
	Data[0] = (uint8_t)(Value & 0xFFU);
	Data[1] = (uint8_t)(Value >> 8);
}

static void QR_WriteLE32(uint8_t *Data, uint32_t Value)
{
	Data[0] = (uint8_t)(Value & 0xFFU);
	Data[1] = (uint8_t)((Value >> 8) & 0xFFU);
	Data[2] = (uint8_t)((Value >> 16) & 0xFFU);
	Data[3] = (uint8_t)((Value >> 24) & 0xFFU);
}

static uint16_t QR_CRC16Volatile(
	const volatile uint8_t *Data,
	uint16_t Length)
{
	uint16_t CRCValue = 0xFFFFU;
	uint16_t i;
	uint8_t Bit;

	for (i = 0U; i < Length; i++)
	{
		CRCValue ^= (uint16_t)Data[i] << 8;
		for (Bit = 0U; Bit < 8U; Bit++)
		{
			if ((CRCValue & 0x8000U) != 0U)
				CRCValue = (uint16_t)((CRCValue << 1) ^ 0x1021U);
			else
				CRCValue <<= 1;
		}
	}
	return CRCValue;
}

static uint32_t QR_CRC32(const uint8_t *Data, uint16_t Length)
{
	uint32_t CRCValue = 0xFFFFFFFFUL;
	uint16_t i;
	uint8_t Bit;

	for (i = 0U; i < Length; i++)
	{
		CRCValue ^= Data[i];
		for (Bit = 0U; Bit < 8U; Bit++)
		{
			if ((CRCValue & 1UL) != 0UL)
				CRCValue = (CRCValue >> 1) ^ 0xEDB88320UL;
			else
				CRCValue >>= 1;
		}
	}
	return CRCValue ^ 0xFFFFFFFFUL;
}

static uint8_t QR_LoadFlashPage(
	uint32_t Address,
	uint8_t *Data,
	uint8_t *Length,
	uint32_t *Sequence)
{
	const uint8_t *Record = (const uint8_t *)Address;
	uint16_t StoredLength;
	uint32_t StoredCRC;
	uint32_t CalculatedCRC;

	if (QR_ReadLE32(Record) != QR_FLASH_MAGIC)
		return 0U;
	if (QR_ReadLE16(Record + 4U) != QR_FLASH_VERSION)
		return 0U;

	StoredLength = QR_ReadLE16(Record + 6U);
	if (StoredLength > QR_MAX_PAYLOAD_LENGTH)
		return 0U;

	StoredCRC = QR_ReadLE32(Record + QR_FLASH_CRC_OFFSET);
	CalculatedCRC = QR_CRC32(
		Record + 4U,
		QR_FLASH_CRC_OFFSET - 4U);
	if (StoredCRC != CalculatedCRC)
		return 0U;

	*Sequence = QR_ReadLE32(Record + 8U);
	*Length = (uint8_t)StoredLength;
	if (StoredLength > 0U)
		memcpy(Data, Record + 12U, StoredLength);
	return 1U;
}

static void QR_LoadAuthorized(void)
{
	uint8_t DataA[QR_MAX_PAYLOAD_LENGTH];
	uint8_t DataB[QR_MAX_PAYLOAD_LENGTH];
	uint8_t LengthA = 0U;
	uint8_t LengthB = 0U;
	uint32_t SequenceA = 0U;
	uint32_t SequenceB = 0U;
	uint8_t ValidA;
	uint8_t ValidB;

	ValidA = QR_LoadFlashPage(
		QR_FLASH_PAGE_A, DataA, &LengthA, &SequenceA);
	ValidB = QR_LoadFlashPage(
		QR_FLASH_PAGE_B, DataB, &LengthB, &SequenceB);

	if (ValidB &&
	    (!ValidA || ((int32_t)(SequenceB - SequenceA) > 0)))
	{
		QR_AuthorizedLength = LengthB;
		QR_StorageSequence = SequenceB;
		QR_ActiveFlashPage = QR_FLASH_PAGE_B;
		memcpy(QR_AuthorizedData, DataB, LengthB);
	}
	else if (ValidA)
	{
		QR_AuthorizedLength = LengthA;
		QR_StorageSequence = SequenceA;
		QR_ActiveFlashPage = QR_FLASH_PAGE_A;
		memcpy(QR_AuthorizedData, DataA, LengthA);
	}
	else
	{
		QR_AuthorizedLength = 0U;
		QR_StorageSequence = 0U;
		QR_ActiveFlashPage = 0U;
	}
}

static uint8_t QR_WriteAuthorized(
	const uint8_t *Data,
	uint8_t Length)
{
	uint8_t Record[QR_FLASH_RECORD_SIZE];
	uint32_t TargetPage;
	uint32_t NewSequence;
	uint16_t Offset;
	uint16_t HalfWord;
	FLASH_Status Status;

	if (Length > QR_MAX_PAYLOAD_LENGTH)
		return 0U;

	memset(Record, 0xFF, sizeof(Record));
	QR_WriteLE32(Record, QR_FLASH_MAGIC);
	QR_WriteLE16(Record + 4U, QR_FLASH_VERSION);
	QR_WriteLE16(Record + 6U, Length);
	NewSequence = QR_StorageSequence + 1UL;
	QR_WriteLE32(Record + 8U, NewSequence);
	if (Length > 0U)
		memcpy(Record + 12U, Data, Length);
	QR_WriteLE32(
		Record + QR_FLASH_CRC_OFFSET,
		QR_CRC32(Record + 4U, QR_FLASH_CRC_OFFSET - 4U));

	TargetPage = (QR_ActiveFlashPage == QR_FLASH_PAGE_A) ?
	             QR_FLASH_PAGE_B : QR_FLASH_PAGE_A;

	FLASH_Unlock();
	FLASH_ClearFlag(
		FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	Status = FLASH_ErasePage(TargetPage);

	/* 先写数据，最后写Magic，断电时不会把半成品当成有效记录。 */
	for (Offset = 4U;
	     (Status == FLASH_COMPLETE) && (Offset < QR_FLASH_RECORD_SIZE);
	     Offset += 2U)
	{
		HalfWord = (uint16_t)Record[Offset] |
		           ((uint16_t)Record[Offset + 1U] << 8);
		Status = FLASH_ProgramHalfWord(TargetPage + Offset, HalfWord);
	}

	if (Status == FLASH_COMPLETE)
	{
		HalfWord = (uint16_t)Record[0] |
		           ((uint16_t)Record[1] << 8);
		Status = FLASH_ProgramHalfWord(TargetPage, HalfWord);
	}
	if (Status == FLASH_COMPLETE)
	{
		HalfWord = (uint16_t)Record[2] |
		           ((uint16_t)Record[3] << 8);
		Status = FLASH_ProgramHalfWord(TargetPage + 2U, HalfWord);
	}
	FLASH_Lock();

	if (Status != FLASH_COMPLETE)
		return 0U;

	QR_AuthorizedLength = Length;
	QR_StorageSequence = NewSequence;
	QR_ActiveFlashPage = TargetPage;
	if (Length > 0U)
		memcpy(QR_AuthorizedData, Data, Length);
	return 1U;
}

static void QR_GPIOInit(void)
{
	GPIO_InitTypeDef GPIOInit;

	RCC_APB2PeriphClockCmd(
		RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOB,
		ENABLE);

	GPIO_PinRemapConfig(GPIO_Remap_I2C1, DISABLE);
	GPIOInit.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIOInit.GPIO_Speed = GPIO_Speed_50MHz;
	GPIOInit.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_Init(GPIOB, &GPIOInit);
}

static void QR_I2CInit(void)
{
	I2C_InitTypeDef I2CInit;
	NVIC_InitTypeDef NVICInit;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	I2C_DeInit(I2C1);

	I2CInit.I2C_ClockSpeed = 100000U;
	I2CInit.I2C_Mode = I2C_Mode_I2C;
	I2CInit.I2C_DutyCycle = I2C_DutyCycle_2;
	I2CInit.I2C_OwnAddress1 = (uint16_t)(QR_I2C_ADDRESS << 1);
	I2CInit.I2C_Ack = I2C_Ack_Enable;
	I2CInit.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_Init(I2C1, &I2CInit);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	NVICInit.NVIC_IRQChannel = I2C1_EV_IRQn;
	NVICInit.NVIC_IRQChannelPreemptionPriority = 1U;
	NVICInit.NVIC_IRQChannelSubPriority = 0U;
	NVICInit.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVICInit);

	NVICInit.NVIC_IRQChannel = I2C1_ER_IRQn;
	NVICInit.NVIC_IRQChannelPreemptionPriority = 1U;
	NVICInit.NVIC_IRQChannelSubPriority = 1U;
	NVICInit.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVICInit);

	I2C_ITConfig(I2C1, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, ENABLE);
	I2C_Cmd(I2C1, ENABLE);
	I2C_AcknowledgeConfig(I2C1, ENABLE);
}

static void QR_AcceptReceivedFrame(void)
{
	uint8_t PayloadLength;
	uint8_t MessageType;
	uint16_t ExpectedLength;
	uint16_t ReceivedCRC;
	uint16_t CalculatedCRC;
	uint8_t i;

	if (QR_RxIndex < QR_FRAME_OVERHEAD)
		return;
	if ((QR_RxFrame[0] != QR_FRAME_HEADER) ||
	    (QR_RxFrame[1] != QR_PROTOCOL_VERSION))
		return;

	MessageType = QR_RxFrame[2];
	PayloadLength = QR_RxFrame[4];
	if (PayloadLength > QR_MAX_PAYLOAD_LENGTH)
		return;

	ExpectedLength = (uint16_t)PayloadLength + QR_FRAME_OVERHEAD;
	if (QR_RxIndex != ExpectedLength)
		return;

	ReceivedCRC = (uint16_t)QR_RxFrame[5U + PayloadLength] |
	              ((uint16_t)QR_RxFrame[6U + PayloadLength] << 8);
	CalculatedCRC = QR_CRC16Volatile(
		&QR_RxFrame[1],
		(uint16_t)PayloadLength + 4U);
	if (ReceivedCRC != CalculatedCRC)
	{
		QR_CRCErrorCount++;
		return;
	}

	if ((MessageType == QR_MSG_DATA) && (PayloadLength > 0U))
	{
		for (i = 0U; i < PayloadLength; i++)
			QR_PendingData[i] = QR_RxFrame[5U + i];
		QR_PendingLength = PayloadLength;
		QR_PendingEvent = QR_EVENT_DATA;
		QR_ValidFrameCount++;
	}
	else if ((MessageType == QR_MSG_CLEAR) && (PayloadLength == 0U))
	{
		QR_PendingLength = 0U;
		QR_PendingEvent = QR_EVENT_CLEAR;
		QR_ValidFrameCount++;
	}
}

void QR_Init(void)
{
	QR_LoadAuthorized();
	QR_GPIOInit();
	QR_I2CInit();
}

uint8_t QR_GetEvent(uint8_t *Data, uint8_t *Length)
{
	uint8_t Event;
	uint8_t LocalLength;
	uint8_t i;
	uint32_t Primask = __get_PRIMASK();

	__disable_irq();
	Event = QR_PendingEvent;
	LocalLength = QR_PendingLength;
	if ((Event == QR_EVENT_DATA) && (Data != 0))
	{
		for (i = 0U; i < LocalLength; i++)
			Data[i] = QR_PendingData[i];
	}
	QR_PendingEvent = QR_EVENT_NONE;
	if (Primask == 0U)
		__enable_irq();

	if (Length != 0)
		*Length = LocalLength;
	return Event;
}

void QR_FlushEvents(void)
{
	uint32_t Primask = __get_PRIMASK();
	__disable_irq();
	QR_PendingEvent = QR_EVENT_NONE;
	QR_PendingLength = 0U;
	if (Primask == 0U)
		__enable_irq();
}

uint8_t QR_HasAuthorized(void)
{
	return (QR_AuthorizedLength > 0U) ? 1U : 0U;
}

uint8_t QR_GetAuthorizedLength(void)
{
	return QR_AuthorizedLength;
}

uint8_t QR_IsAuthorized(const uint8_t *Data, uint8_t Length)
{
	if ((Length == 0U) || (Length != QR_AuthorizedLength))
		return 0U;
	return (memcmp(Data, QR_AuthorizedData, Length) == 0) ? 1U : 0U;
}

uint8_t QR_SaveAuthorized(const uint8_t *Data, uint8_t Length)
{
	if ((Data == 0) || (Length == 0U))
		return 0U;
	return QR_WriteAuthorized(Data, Length);
}

uint8_t QR_ClearAuthorized(void)
{
	return QR_WriteAuthorized(QR_AuthorizedData, 0U);
}

void I2C1_EV_IRQHandler(void)
{
	uint32_t SR1 = I2C1->SR1;

	if ((SR1 & I2C_SR1_ADDR) != 0U)
	{
		volatile uint32_t ClearAddress;
		ClearAddress = I2C1->SR1;
		ClearAddress = I2C1->SR2;
		(void)ClearAddress;
		QR_RxIndex = 0U;
	}

	while ((I2C1->SR1 & I2C_SR1_RXNE) != 0U)
	{
		uint8_t Byte = (uint8_t)I2C1->DR;
		if (QR_RxIndex < QR_RX_BUFFER_LENGTH)
			QR_RxFrame[QR_RxIndex++] = Byte;
		else
			QR_RxIndex = QR_RX_BUFFER_LENGTH + 1U;
	}

	if ((I2C1->SR1 & I2C_SR1_STOPF) != 0U)
	{
		volatile uint32_t ClearStop = I2C1->SR1;
		(void)ClearStop;
		I2C1->CR1 |= I2C_CR1_PE;
		QR_AcceptReceivedFrame();
	}
}

void I2C1_ER_IRQHandler(void)
{
	uint32_t Errors = I2C1->SR1 &
	                  (I2C_SR1_BERR | I2C_SR1_ARLO |
	                   I2C_SR1_AF | I2C_SR1_OVR);
	I2C1->SR1 &= ~Errors;
	QR_RxIndex = 0U;
	QR_I2CErrorCount++;
	I2C_AcknowledgeConfig(I2C1, ENABLE);
}
