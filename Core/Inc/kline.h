#ifndef __KLINE_H
#define __KLINE_H
#include "main.h"
#include <stdint.h>

/* ==================== Сервис ==================== */
void     KLine_Init(void);
void     KLine_SetBaud(uint32_t baud);
uint32_t KLine_GetBaud(void);
void     KLine_SetWake(uint8_t on);
void     KLine_Wakeup(void);
void     KLine_Send5Baud(uint8_t addr);
void     KLine_Flush(void);
uint16_t KLine_Read(uint8_t *out, uint16_t max);

/* ==================== Рукопожатие ==================== */
uint8_t  KLine_KWP1281_Connect(uint8_t addr, uint8_t mode, uint8_t *kb1, uint8_t *kb2);
uint8_t  KLine_KWP1281_Dialog(uint8_t addr, uint8_t *kb1, uint8_t *kb2);

/* ==================== Блоки KWP1281 ==================== */
uint8_t  KWP1281_RecvBlock(uint8_t *title, uint8_t *data, uint8_t cap,
                           uint8_t *dlen, uint32_t timeout_ms);
uint8_t  KWP1281_SendBlock(uint8_t title, const uint8_t *data, uint8_t n);
uint8_t  KWP1281_ReadIdent(char *str, uint8_t cap, uint8_t *len);
uint8_t  KWP1281_ReadGroup(uint8_t group, uint8_t *resp, uint8_t *rlen);
uint8_t  KWP1281_ReadDTC(uint8_t *resp, uint8_t *rlen);
uint8_t  KWP1281_ClearDTC(void);
uint8_t  KWP1281_End(void);
uint8_t  KWP1281_AckBlock(void);

/* ==================== Результат измерений ==================== */
typedef struct {
    uint8_t  ok;
    uint8_t  group;
    float    val[4];
    char     unit[4][12];
    uint8_t  type[4];
    uint16_t raw[4];
} KLineMeas_t;

/* ==================== LIVE-поток блоков измерений ==================== */
uint8_t KLine_MeasStream_Start(uint8_t addr);              /* открыть сессию */
void    KLine_MeasStream_Stop(void);                       /* End + закрыть */
uint8_t KLine_MeasStream_IsOpen(void);
uint8_t KLine_MeasStream_Poll(uint8_t group, KLineMeas_t *out);

/* ==================== Список ЭБУ ==================== */
#define KLINE_ECU_COUNT 6
typedef struct {
    uint8_t     addr;
    const char *name;
} KLineEcu_t;
extern const KLineEcu_t KLINE_ECU_LIST[KLINE_ECU_COUNT];

/* ==================== DTC ==================== */
typedef struct {
    uint16_t code;
    uint8_t  type;
    char     txt[48];
} KLineDtc_t;
#define KLINE_DTC_MAX 16
typedef struct {
    uint8_t    count;
    KLineDtc_t list[KLINE_DTC_MAX];
} KLineDtcList_t;

uint8_t KLine_GetDtcList(uint8_t addr, KLineDtcList_t *out);
uint8_t KLine_ClearDtcs (uint8_t addr);

#endif
