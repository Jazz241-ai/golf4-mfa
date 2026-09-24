#ifndef __KLINE_H
#define __KLINE_H
#include "main.h"
#include <stdint.h>

/* ==================== Сервис ==================== */
void     KLine_Init(void);
void     KLine_SetBaud(uint32_t baud);
uint32_t KLine_GetBaud(void);
void     KLine_SetWake(uint8_t on);        /* 0 = чистый 5-baud (рабочий режим) */
void     KLine_Wakeup(void);
void     KLine_Send5Baud(uint8_t addr);    /* 7O1, 200 мс/бит */
void     KLine_Flush(void);
uint16_t KLine_Read(uint8_t *out, uint16_t max);

/* ==================== Рукопожатие ==================== */
/* mode: 0 = ~KB2 (стандарт), 1 = KB2, 2 = без ACK. Возврат 1 = успех */
uint8_t  KLine_KWP1281_Connect(uint8_t addr, uint8_t mode, uint8_t *kb1, uint8_t *kb2);
/* 0 = успех, 3 = провал */
uint8_t  KLine_KWP1281_Dialog(uint8_t addr, uint8_t *kb1, uint8_t *kb2);

/* ==================== Блоки KWP1281 ==================== */
/* [Len][Counter][Title][Data...][0x03], побайтовый ACK инверсией */
uint8_t  KWP1281_RecvBlock(uint8_t *title, uint8_t *data, uint8_t cap,
                           uint8_t *dlen, uint32_t timeout_ms);
uint8_t  KWP1281_SendBlock(uint8_t title, const uint8_t *data, uint8_t n);
uint8_t  KWP1281_ReadIdent(char *str, uint8_t cap, uint8_t *len);        /* 0xF6 */
uint8_t  KWP1281_ReadGroup(uint8_t group, uint8_t *resp, uint8_t *rlen); /* 0x29 -> 0xE7 */
uint8_t  KWP1281_ReadDTC(uint8_t *resp, uint8_t *rlen);                  /* 0x07 -> 0xFC */
uint8_t  KWP1281_ClearDTC(void);                                         /* 0x05 */
uint8_t  KWP1281_End(void);                                              /* 0x06 */
uint8_t  KWP1281_AckBlock(void);                                    /* 0x09 */

/* ==================== Список ЭБУ ==================== */
#define KLINE_ECU_COUNT 6
typedef struct {
    uint8_t     addr;
    const char *name;
} KLineEcu_t;
extern const KLineEcu_t KLINE_ECU_LIST[KLINE_ECU_COUNT];

/* ==================== Результаты измерений ==================== */
typedef struct {
    uint8_t  ok;
    uint8_t  group;
    float    val[4];
    char     unit[4][12];
    uint8_t  type[4];    /* <-- байт типа канала (для отладки)   */
    uint16_t raw[4];     /* <-- сырое 16-бит значение (калибровка) */
} KLineMeas_t;

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

/* ==================== Высокий уровень ==================== */
uint8_t KLine_GetMeasGroup(uint8_t addr, uint8_t group, KLineMeas_t *out);
uint8_t KLine_GetDtcList  (uint8_t addr, KLineDtcList_t *out);
uint8_t KLine_ClearDtcs   (uint8_t addr);

#endif
