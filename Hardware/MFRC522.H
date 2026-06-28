#ifndef __MFRC522_H
#define __MFRC522_H

/* ═══════════════════════════════════════════════════════════
 *  MFRC522.H — RC522 IC卡模块 驱动头文件
 *
 *  包含：外部函数声明、RC522内部命令字、Mifare卡命令字、
 *        寄存器地址定义、错误码
 * ═══════════════════════════════════════════════════════════ */

/* ── 外部函数声明（实现在 RC522.C） ────────────────────── */

// 模块控制
extern char PcdReset(void);                                    // 复位RC522
extern void PcdAntennaOn(void);                                // 打开天线
extern void PcdAntennaOff(void);                               // 关闭天线
extern char M500PcdConfigISOType(unsigned char type);          // 配置ISO协议类型（'A'=14443A）

// 卡片操作（ISO14443-3 防冲突 + 选卡流程）
extern char PcdRequest(unsigned char req_code,unsigned char *pTagType);   // 寻卡（REQA/WUPA）
extern char PcdAnticoll(unsigned char *pSnr);                             // 防冲突，获取4字节序列号
extern char PcdSelect(unsigned char *pSnr);                               // 选定卡片

// 卡片认证 & 数据读写（ISO14443-4 / Mifare 协议）
extern char PcdAuthState(unsigned char auth_mode,unsigned char addr,unsigned char *pKey,unsigned char *pSnr); // 验证密码
extern char PcdRead(unsigned char addr,unsigned char *pData);              // 读一个块（16字节）
extern char PcdWrite(unsigned char addr,unsigned char *pData);             // 写一个块（16字节）
extern char PcdValue(unsigned char dd_mode,unsigned char addr,unsigned char *pValue);   // 充值/扣款
extern char PcdBakValue(unsigned char sourceaddr, unsigned char goaladdr);               // 备份块数据
extern char PcdHalt(void);                                                 // 让卡片进入休眠

// 底层通信 & 工具函数
extern char PcdComMF522(unsigned char Command,                             // RC522核心通信函数
                 unsigned char *pInData, unsigned char InLenByte,
                 unsigned char *pOutData, unsigned int  *pOutLenBit);
extern void CalulateCRC(unsigned char *pIndata,unsigned char len,unsigned char *pOutData); // CRC16校验
extern void WriteRawRC(unsigned char Address,unsigned char value);         // 写RC522寄存器
extern unsigned char ReadRawRC(unsigned char Address);                     // 读RC522寄存器
extern void SetBitMask(unsigned char reg,unsigned char mask);              // 寄存器某位置1
extern void ClearBitMask(unsigned char reg,unsigned char mask);            // 寄存器某位清0
extern void delay_10ms(unsigned int _10ms);                                // 延时函数（循环空转）
extern void WaitCardOff(void);                                             // 等待卡片拿开


/* ── MF522 内部命令字（写到 CommandReg 寄存器） ────── */
#define PCD_IDLE              0x00               // 取消当前命令
#define PCD_AUTHENT           0x0E               // 验证密钥
#define PCD_RECEIVE           0x08               // 接收数据
#define PCD_TRANSMIT          0x04               // 发送数据
#define PCD_TRANSCEIVE        0x0C               // 发送并接收数据（最常用）
#define PCD_RESETPHASE        0x0F               // 复位
#define PCD_CALCCRC           0x03               // CRC计算


/* ── Mifare One 卡片命令字（通过RC522转发给卡） ──── */
#define PICC_REQIDL           0x26               // 寻天线区内未进入休眠状态（REQA）
#define PICC_REQALL           0x52               // 寻天线区内全部卡片（WUPA）
#define PICC_ANTICOLL1        0x93               // 防冲突1级
#define PICC_ANTICOLL2        0x95               // 防冲突2级
#define PICC_AUTHENT1A        0x60               // 验证A密钥
#define PICC_AUTHENT1B        0x61               // 验证B密钥
#define PICC_READ             0x30               // 读一个块
#define PICC_WRITE            0xA0               // 写一个块
#define PICC_DECREMENT        0xC0               // 扣款
#define PICC_INCREMENT        0xC1               // 充值
#define PICC_RESTORE          0xC2               // 调块数据到缓冲区
#define PICC_TRANSFER         0xB0               // 保存缓冲区中数据到卡
#define PICC_HALT             0x50               // 休眠


/* ── 常用宏 ────────────────────────────────────── */
#define DEF_FIFO_LENGTH       64                 // RC522内部FIFO缓冲区大小：64字节
#define REQ_ALL               0x52               // 寻卡方式：寻所有卡（=PICC_REQALL）
#define KEYA                  0x60               // 验证A密钥（=PICC_AUTHENT1A）


/* ── RC522 寄存器地址（4页 × 16个 = 64个寄存器） ──
 * 读写地址时，WriteRawRC用 (addr<<1)&0x7E
 *              ReadRawRC 用 ((addr<<1)&0x7E)|0x80
 * ────────────────────────────────────────────────── */
// PAGE 0（基础控制寄存器）
#define     RFU00                 0x00          // 保留
#define     CommandReg            0x01          // 命令寄存器（启停RC522命令）
#define     ComIEnReg             0x02          // 通信中断使能
#define     DivlEnReg             0x03          // 内部中断使能
#define     ComIrqReg             0x04          // 通信中断标志
#define     DivIrqReg             0x05          // 内部中断标志
#define     ErrorReg              0x06          // 错误标志
#define     Status1Reg            0x07          // 状态寄存器1
#define     Status2Reg            0x08          // 状态寄存器2（含加密状态）
#define     FIFODataReg           0x09          // FIFO数据读写端口
#define     FIFOLevelReg          0x0A          // FIFO已用字节数
#define     WaterLevelReg         0x0B          // FIFO水位线
#define     ControlReg            0x0C          // 控制寄存器
#define     BitFramingReg         0x0D          // 位帧调整
#define     CollReg               0x0E          // 冲突检测
#define     RFU0F                 0x0F
// PAGE 1（发送/接收配置）
#define     RFU10                 0x10
#define     ModeReg               0x11          // 模式寄存器（收发模式）
#define     TxModeReg             0x12          // 发送模式
#define     RxModeReg             0x13          // 接收模式
#define     TxControlReg          0x14          // 发送控制（天线开关也在这）
#define     TxAutoReg             0x15          // 自动发送配置
#define     TxSelReg              0x16          // 发送选择
#define     RxSelReg              0x17          // 接收选择
#define     RxThresholdReg        0x18          // 接收阈值
#define     DemodReg              0x19          // 解调配置
#define     RFU1A                 0x1A
#define     RFU1B                 0x1B
#define     MifareReg             0x1C          // Mifare 专用配置
#define     RFU1D                 0x1D
#define     RFU1E                 0x1E
#define     SerialSpeedReg        0x1F          // 串行速率
// PAGE 2（CRC & 定时器）
#define     RFU20                 0x20
#define     CRCResultRegM         0x21          // CRC结果高字节
#define     CRCResultRegL         0x22          // CRC结果低字节
#define     RFU23                 0x23
#define     ModWidthReg           0x24          // 调制宽度
#define     RFU25                 0x25
#define     RFCfgReg              0x26          // RF配置
#define     GsNReg                0x27
#define     CWGsCfgReg            0x28
#define     ModGsCfgReg           0x29
#define     TModeReg              0x2A          // 定时器模式
#define     TPrescalerReg         0x2B          // 定时器预分频
#define     TReloadRegH           0x2C          // 定时器重装值高字节
#define     TReloadRegL           0x2D          // 定时器重装值低字节
#define     TCounterValueRegH     0x2E          // 定时器计数值高
#define     TCounterValueRegL     0x2F          // 定时器计数值低
// PAGE 3（测试寄存器，正常不用）
#define     RFU30                 0x30
#define     TestSel1Reg           0x31
#define     TestSel2Reg           0x32
#define     TestPinEnReg          0x33
#define     TestPinValueReg       0x34
#define     TestBusReg            0x35
#define     AutoTestReg           0x36
#define     VersionReg            0x37          // 版本号（通常读0x92或0x91）
#define     AnalogTestReg         0x38
#define     TestDAC1Reg           0x39
#define     TestDAC2Reg           0x3A
#define     TestADCReg            0x3B
#define     RFU3C                 0x3C
#define     RFU3D                 0x3D
#define     RFU3E                 0x3E
#define     RFU3F                 0x3F


/* ── 错误码（PcdXxx 系列函数的返回值） ─────────── */
#define MI_OK                          (char)0       // 操作成功
#define MI_NOTAGERR                    (char)(-1)    // 没有检测到卡片
#define MI_ERR                         (char)(-2)    // 通信错误

#endif