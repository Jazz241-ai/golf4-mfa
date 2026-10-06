#include "kline.h"
#include <string.h>
#include <stdio.h>
#include "stm32h7xx_hal.h"

extern UART_HandleTypeDef huart2;

/* ==========================================================================
   КОНФИГУРАЦИЯ И БУФЕРЫ
========================================================================== */
#define RX_SZ 512
static uint8_t           rx_buf[RX_SZ];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;
static uint8_t           rx_byte_tmp;
static uint8_t           wake_enable = 0;
static uint8_t           blk_counter = 0;
static uint32_t          kline_baud  = 10400;

// 0 = только складываем в буфер (Init и SendBlock)
// 1 = автоматически отвечаем инверсией (RecvBlock)
static volatile uint8_t  auto_ack_mode = 0;

/* ==========================================================================
   RX ISR
========================================================================== */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        uint8_t received = rx_byte_tmp;
        if (auto_ack_mode) {
            uint8_t ack = (uint8_t)~received;
            while (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TXE) == RESET);
            huart2.Instance->TDR = ack;
            uint32_t t0 = HAL_GetTick();
            while (HAL_GetTick() - t0 < 2) {
                if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE)) {
                    uint8_t echo = (uint8_t)(huart2.Instance->RDR & 0xFF);
                    if (echo == ack) break;
                }
            }
        }
        rx_buf[rx_head++ & (RX_SZ - 1)] = received;
        if (HAL_UART_Receive_IT(&huart2, &rx_byte_tmp, 1) != HAL_OK) {
            __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);
        }
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        volatile uint8_t tmp = (uint8_t)(huart2.Instance->RDR & 0xFF);
        (void)tmp;
        __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF | UART_CLEAR_FEF | UART_CLEAR_NEF | UART_CLEAR_PEF);
        if (HAL_UART_Receive_IT(&huart2, &rx_byte_tmp, 1) != HAL_OK) {
            __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);
        }
    }
}

static void rx_start(void) {
    if (HAL_UART_Receive_IT(&huart2, &rx_byte_tmp, 1) != HAL_OK) {
        __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);
    }
}

/* ==========================================================================
   INIT / BAUD
========================================================================== */
void KLine_Init(void) {
    HAL_UART_AbortReceive_IT(&huart2);
    HAL_UART_AbortTransmit_IT(&huart2);

    huart2.Init.BaudRate     = 10400;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.AdvancedInit.AdvFeatureInit   = UART_ADVFEATURE_RXINVERT_INIT;
    huart2.AdvancedInit.RxPinLevelInvert = UART_ADVFEATURE_RXINV_DISABLE;

    HAL_UART_Init(&huart2);
    huart2.gState  = HAL_UART_STATE_READY;
    huart2.RxState = HAL_UART_STATE_READY;

    rx_head = rx_tail = 0;
    auto_ack_mode = 0;
    rx_start();
}

void KLine_SetBaud(uint32_t baud) {
    HAL_UART_AbortReceive_IT(&huart2);
    HAL_UART_AbortTransmit_IT(&huart2);
    huart2.Init.BaudRate = baud;
    HAL_UART_Init(&huart2);
    huart2.gState  = HAL_UART_STATE_READY;
    huart2.RxState = HAL_UART_STATE_READY;
    rx_head = rx_tail = 0;
    rx_start();
}

uint32_t KLine_GetBaud(void) { return kline_baud; }
void KLine_SetWake(uint8_t on) { wake_enable = on; }

/* ==========================================================================
   PIN CONTROL
========================================================================== */
static void pin_to_gpio(void) {
    GPIO_InitTypeDef g = {0};
    HAL_UART_AbortReceive_IT(&huart2);
    HAL_UART_AbortTransmit_IT(&huart2);

    g.Pin   = GPIO_PIN_2;
    g.Mode  = GPIO_MODE_OUTPUT_OD;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &g);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);

    rx_head = rx_tail = 0;
}

static void pin_to_uart(void) {
    GPIO_InitTypeDef g = {0};
    g.Pin       = GPIO_PIN_2;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &g);

    huart2.gState  = HAL_UART_STATE_READY;
    huart2.RxState = HAL_UART_STATE_READY;
    rx_start();
}

void KLine_Wakeup(void) {
    pin_to_gpio();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
    HAL_Delay(300);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
    pin_to_uart();
}

/* ==========================================================================
   5-BAUD INIT
========================================================================== */
void KLine_Send5Baud(uint8_t addr) {
    pin_to_gpio();

    if (wake_enable) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
        HAL_Delay(300);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
        HAL_Delay(300);
    } else {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
        HAL_Delay(300);
    }

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
    HAL_Delay(200);

    uint8_t ones = 0;
    for (uint8_t i = 0; i < 7; i++) {
        uint8_t b = (addr >> i) & 1;
        ones += b;
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, b ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_Delay(200);
    }

    uint8_t p = (ones & 1) ? 0 : 1;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, p ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_Delay(200);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
    HAL_Delay(200);

    pin_to_uart();
}

/* ==========================================================================
   PRIMITIVES (TX/RX) - ПЕРЕМЕЩЕНО ВЫШЕ RecvBlock
========================================================================== */
static int16_t ring_get(uint32_t ms) {
    uint32_t t0 = HAL_GetTick();
    while (HAL_GetTick() - t0 < ms) {
        if (rx_tail != rx_head) return rx_buf[rx_tail++ & (RX_SZ - 1)];
    }
    return -1;
}

static int16_t ring_get_nz(uint32_t ms) {
    uint32_t t0 = HAL_GetTick();
    while (HAL_GetTick() - t0 < ms) {
        if (rx_tail != rx_head) {
            uint8_t c = rx_buf[rx_tail++ & (RX_SZ - 1)];
            if (c != 0x00) return c;
        }
    }
    return -1;
}

void KLine_Flush(void) {
    __disable_irq();
    rx_head = rx_tail = 0;
    __enable_irq();
}

uint16_t KLine_Read(uint8_t *out, uint16_t max) {
    uint16_t n = 0;
    while (rx_tail != rx_head && n < max) out[n++] = rx_buf[rx_tail++ & (RX_SZ - 1)];
    return n;
}

static void tx_byte(uint8_t b) {
    if (huart2.gState != HAL_UART_STATE_READY) huart2.gState = HAL_UART_STATE_READY;
    if (HAL_UART_Transmit(&huart2, &b, 1, 50) != HAL_OK) {
        huart2.gState = HAL_UART_STATE_READY;
        HAL_UART_Transmit(&huart2, &b, 1, 50);
    }
    uint32_t t0 = HAL_GetTick();
    while (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TC) == RESET) {
        if (HAL_GetTick() - t0 > 50) break;
    }
}

static void tx_drain_echo(uint8_t b, uint32_t ms) {
    uint32_t t0 = HAL_GetTick();
    while (HAL_GetTick() - t0 < ms) {
        if (rx_tail != rx_head) {
            uint8_t c = rx_buf[rx_tail++ & (RX_SZ - 1)];
            if (c == b) return;
        }
    }
}

// Передача байта с ожиданием ACK от ЭБУ (инверсии)
static uint8_t tx_byte_ack(uint8_t b, uint32_t ms) {
    tx_byte(b);
    uint32_t t0 = HAL_GetTick();
    while (HAL_GetTick() - t0 < ms) {
        if (rx_tail != rx_head) {
            uint8_t c = rx_buf[rx_tail++ & (RX_SZ - 1)];
            if (c == b) continue;       // Игнорируем эхо
            if (c == (uint8_t)~b) return 1; // Получили ACK от ЭБУ
        }
    }
    return 0;
}

// Прием байта (ACK уже отправлен в ISR, если auto_ack_mode=1)
// ОПРЕДЕЛЕНИЕ ДО RecvBlock, чтобы избежать ошибки компиляции
static int16_t rx_byte_auto_ack(uint8_t *b, uint32_t ms) {
    int16_t c = ring_get(ms);
    if (c < 0) return -1;
    *b = (uint8_t)c;
    return c;
}

/* ==========================================================================
   KWP1281 PROTOCOL BLOCKS
========================================================================== */
uint8_t KLine_KWP1281_Connect(uint8_t addr, uint8_t mode, uint8_t *kb1, uint8_t *kb2) {
    static const uint32_t bauds[3] = {10400, 9600, 4800};

    for (uint8_t bi = 0; bi < 3; bi++) {
        KLine_SetBaud(bauds[bi]);
        KLine_Flush();
        auto_ack_mode = 0; // Строго 0 для инициализации

        KLine_Send5Baud(addr);

        int16_t c = -1;
        uint32_t t0 = HAL_GetTick();
        while (HAL_GetTick() - t0 < 1500) {
            c = ring_get_nz(50);
            if (c == 0x55) break;
        }
        if (c != 0x55) continue;

        int16_t c1 = ring_get_nz(300);
        int16_t c2 = ring_get_nz(300);
        if (c1 < 0 || c2 < 0) continue;

        *kb1 = (uint8_t)c1;
        *kb2 = (uint8_t)c2;

        if (mode != 2) {
            uint8_t ack = (mode == 0) ? (uint8_t)~(*kb2) : *kb2;
            HAL_Delay(5);
            tx_byte(ack);
            tx_drain_echo(ack, 20);
        }

        kline_baud  = bauds[bi];
        blk_counter = 0;
        return 1;
    }
    return 0;
}

/*uint8_t KLine_KWP1281_Dialog(uint8_t addr, uint8_t *kb1, uint8_t *kb2) {
    if (KLine_KWP1281_Connect(addr, 0, kb1, kb2)) return 0;
    HAL_Delay(200);
    if (KLine_KWP1281_Connect(addr, 0, kb1, kb2)) return 0;
    return 3;
}*/

uint8_t KWP1281_RecvBlock(uint8_t *title, uint8_t *data, uint8_t cap, uint8_t *dlen, uint32_t timeout_ms) {
    uint8_t L, ctr, t, b;
    auto_ack_mode = 1;

    if (rx_byte_auto_ack(&L, timeout_ms) < 0)   { auto_ack_mode = 0; return 0; }
    if (L < 3 || L > 63)                        { auto_ack_mode = 0; return 0; }
    if (rx_byte_auto_ack(&ctr, timeout_ms) < 0) { auto_ack_mode = 0; return 0; }
    blk_counter = ctr;
    if (rx_byte_auto_ack(&t, timeout_ms) < 0)   { auto_ack_mode = 0; return 0; }

    uint8_t nd = L - 3, got = 0;
    for (uint8_t i = 0; i < nd; i++) {
        if (rx_byte_auto_ack(&b, timeout_ms) < 0) { auto_ack_mode = 0; return 0; }
        if (data && got < cap) data[got++] = b;
    }

    auto_ack_mode = 0;                 /* 0x03 не ACKуется */
    int16_t c = ring_get(timeout_ms);
    if (c < 0) return 0;
    b = (uint8_t)c;
    if (b != 0x03) return 0;

    if (title) *title = t;
    if (dlen)  *dlen  = got;
    return 1;
}

uint8_t KWP1281_SendBlock(uint8_t title, const uint8_t *data, uint8_t n) {
    // ВАЖНО: При передаче авто-ответ должен быть ВЫКЛЮЧЕН
    auto_ack_mode = 0;

    blk_counter++;
    uint8_t L = n + 3;

    if (!tx_byte_ack(L, 300)) return 0;
    HAL_Delay(5);

    if (!tx_byte_ack(blk_counter, 300)) return 0;
    HAL_Delay(5);

    if (!tx_byte_ack(title, 300)) return 0;

    for (uint8_t i = 0; i < n; i++) {
        HAL_Delay(5);
        if (!tx_byte_ack(data[i], 300)) return 0;
    }

    HAL_Delay(5);
    tx_byte(0x03); // End marker
    tx_drain_echo(0x03, 20);

    return 1;
}

uint8_t KWP1281_ReadIdent(char *str, uint8_t cap, uint8_t *len) {
    uint8_t total = 0;
    uint8_t prompted = 0;

    for (uint8_t b = 0; b < 10; b++) {
        uint8_t t = 0, dl = 0;
        uint8_t data[32];

        /* 1) пассивно ждём следующий блок 0xF6 (пауза между блоками может быть >60 мс) */
        if (!KWP1281_RecvBlock(&t, data, 32, &dl, b ? 150 : 800)) {
            /* 2) тишина: если данные уже есть и мы ещё не "подталкивали" ЭБУ после
                   последнего блока — шлём Ack (0x09) и ждём ещё раз */
            if (total && !prompted) {
                prompted = 1;
                KWP1281_AckBlock();
                if (!KWP1281_RecvBlock(&t, data, 32, &dl, 150)) break;
            } else {
                break;   /* идентификатор закончился */
            }
        }

        if (t != 0xF6) break;   /* пришёл не-ASCII блок — ident завершён */

        for (uint8_t i = 0; i < dl && total + 1 < cap; i++)
            str[total++] = (char)data[i];

        prompted = 0;   /* после каждого нового блока снова разрешаем один Ack */
    }

    str[total] = 0;
    *len = total;
    return total > 0;
}

uint8_t KWP1281_ReadGroup(uint8_t group, uint8_t *resp, uint8_t *rlen) {
    KLine_Flush();                      /* линия после ident-таймаута тихая — чистим кольцо */
    HAL_Delay(5);
    if (!KWP1281_SendBlock(0x29, &group, 1)) return 0;
    uint8_t t = 0, dl = 0;
    if (!KWP1281_RecvBlock(&t, resp, 32, &dl, 1000)) return 0;
    if (t != 0xE7) return 0;
    *rlen = dl;
    return 1;
}

uint8_t KWP1281_ReadDTC(uint8_t *resp, uint8_t *rlen) {
    HAL_Delay(5);
    if (!KWP1281_SendBlock(0x07, NULL, 0)) return 0;

    uint8_t t = 0, dl = 0;
    if (!KWP1281_RecvBlock(&t, resp, 48, &dl, 2000)) return 0;
    *rlen = dl;
    return (t == 0xFC);
}

uint8_t KWP1281_ClearDTC(void) {
    HAL_Delay(5);
    return KWP1281_SendBlock(0x05, NULL, 0);
}

uint8_t KWP1281_End(void) {
    HAL_Delay(5);
    return KWP1281_SendBlock(0x06, NULL, 0);
}

uint8_t KWP1281_AckBlock(void) {
    HAL_Delay(5);
    return KWP1281_SendBlock(0x09, NULL, 0);
}

const KLineEcu_t KLINE_ECU_LIST[KLINE_ECU_COUNT] = {
    { 0x01, "Двигатель" }, { 0x03, "АКПП" }, { 0x15, "Airbag" },
    { 0x17, "Приборка" }, { 0x19, "CAN-шлюз" }, { 0x46, "Комфорт" },
};

/* Масштабы каналов KWP1281 (best-known, калибровать по VCDS):
   raw = a*256 + b (16 бит, big-endian)                          */
/* Коды типов каналов KWP1281.
 * ПОДТВЕРЖДЕНО сверкой со сканером ("Лаунч") на Golf 4 (ME7.5):
 *   0x01 = об/мин   (raw * 0.25)
 *   0x05 = °C       (raw * 0.75 - 48)   <- раньше ошибочно считали Вольтами
 *   0x15 = В        (raw * 0.001)       <- раньше ошибочно считали барами
 * Остальные коды - предварительные, калибровать по RAW-строке на экране.
 */
/* KWP1281: 1-й байт канала = ДЕСЯТИЧНЫЙ показатель (1..70).
 * a = 2-й байт, b = 3-й. Формулы — таблица blafusel/opendiag ( = проверено автором). */
static void parse_meas_field(uint8_t type, uint8_t a, uint8_t b, float *v, char *unit) {
    float fa = a, fb = b;
    switch (type) {
    case 1:  *v = 0.2f*fa*fb;                 snprintf(unit,12,"об/мин"); break; /*  */
    case 2:  *v = fa*0.002f*fb;               snprintf(unit,12,"%");      break; /*  нагрузка */
    case 3:  *v = 0.002f*fa*fb;               snprintf(unit,12,"°");      break;
    case 4:  *v = ((fb>127)?fb-127:127-fb)*0.01f*fa; snprintf(unit,12,(fb>127)?"ATDC":"BTDC"); break;
    case 5:  *v = fa*(fb-100.0f)*0.1f;        snprintf(unit,12,"°C");     break; /*  */
    case 6:  *v = 0.001f*fa*fb;               snprintf(unit,12,"В");      break; /*  */
    case 7:  *v = 0.01f*fa*fb;                snprintf(unit,12,"км/ч");   break; /*  */
    case 8:  *v = 0.1f*fa*fb;                 snprintf(unit,12,"");       break;
    case 9:  *v = (fb-127.0f)*0.02f*fa;       snprintf(unit,12,"°");      break;
    case 10: *v = (b==0)?0:1;                 snprintf(unit,12,(b==0)?"COLD":"WARM"); break;
    case 11: *v = 0.0001f*fa*(fb-128.0f)+1.0f;snprintf(unit,12,"-");      break;
    case 12: *v = 0.001f*fa*fb;               snprintf(unit,12,"Ом");     break;
    case 13: *v = (fb-127.0f)*0.001f*fa;      snprintf(unit,12,"мм");     break;
    case 14: *v = 0.005f*fa*fb;               snprintf(unit,12,"бар");    break;
    case 15: *v = 0.01f*fa*fb;                snprintf(unit,12,"мс");     break;
    case 16: *v = b;                          snprintf(unit,12,"биты");   break;
    case 17: *v = 0;                          snprintf(unit,12,"chr");    break;
    case 18: *v = 0.04f*fa*fb;                snprintf(unit,12,"мбар");   break;
    case 19: *v = fa*fb*0.01f;                snprintf(unit,12,"л");      break; /*  */
    case 20: *v = fa*(fb-128.0f)/128.0f;      snprintf(unit,12,"%");      break;
    case 21: *v = 0.001f*fa*fb;               snprintf(unit,12,"В");      break;
    case 22: *v = 0.001f*fa*fb;               snprintf(unit,12,"мс");     break;
    case 23: *v = fb/256.0f*fa;               snprintf(unit,12,"%");      break;
    case 24: *v = 0.001f*fa*fb;               snprintf(unit,12,"А");      break;
    case 25: *v = (fb*1.421f)+(fa/182.0f);    snprintf(unit,12,"г/с");    break;
    case 26: *v = fb-fa;                      snprintf(unit,12,"C");      break;
    case 27: *v = ((fb>128)?fb-128:128-fb)*0.01f*fa; snprintf(unit,12,(fb<128)?"ATDC":"BTDC"); break;
    case 28: *v = fb-fa;                      snprintf(unit,12,"-");      break;
    case 29: *v = (b<a)?1:2;                  snprintf(unit,12,(b<a)?"KF1":"KF2"); break;
    case 30: *v = fb/12.0f*fa;                snprintf(unit,12,"°кв");    break;
    case 31: *v = fb/2560.0f*fa;              snprintf(unit,12,"°C");     break;
    case 32: *v = (b>128)?(float)(int8_t)b:fb; snprintf(unit,12,"-");     break;
    case 33: *v = (a==0)?100.0f*fb:100.0f*fb/fa; snprintf(unit,12,"%");   break; /*  */
    case 34: *v = (fb-128.0f)*0.01f*fa;       snprintf(unit,12,"кВт");    break;
    case 35: *v = 0.01f*fa*fb;                snprintf(unit,12,"л/ч");    break; /*  */
    case 36: *v = fa*2560.0f+fb*10.0f;        snprintf(unit,12,"км");     break; /*  пробег */
    case 37: *v = 0;                          snprintf(unit,12,"OilPr"); break;
    case 38: *v = (fb-128.0f)*0.001f*fa;      snprintf(unit,12,"°кв");    break;
    case 39: *v = fb/256.0f*fa;               snprintf(unit,12,"мг/ч");   break;
    case 40: *v = fb*0.1f+(25.5f*fa)-400.0f;  snprintf(unit,12,"А");      break;
    case 41: *v = fb+fa*255.0f;               snprintf(unit,12,"Ач");     break;
    case 42: *v = fb*0.1f+(25.5f*fa)-400.0f;  snprintf(unit,12,"кВт");    break;
    case 43: *v = fb*0.1f+(25.5f*fa);         snprintf(unit,12,"В");      break;
    case 44: *v = fa*100.0f+fb;               snprintf(unit,12,"ч:м");    break; /*  */
    case 45: *v = 0.1f*fa*fb/100.0f;          snprintf(unit,12,"");       break;
    case 46: *v = (fa*fb-3200.0f)*0.0027f;    snprintf(unit,12,"°кв");    break;
    case 47: *v = (fb-128.0f)*fa;             snprintf(unit,12,"мс");     break;
    case 48: *v = fb+fa*255.0f;               snprintf(unit,12,"-");      break;
    case 49: *v = (fb/4.0f)*fa*0.1f;          snprintf(unit,12,"мг/ч");   break;
    case 50: *v = (a==0)?(fb-128.0f)/0.01f:(fb-128.0f)/(0.01f*fa); snprintf(unit,12,"мбар"); break;
    case 51: *v = ((fb-128.0f)/255.0f)*fa;    snprintf(unit,12,"мг/ч");   break;
    case 52: *v = fb*0.02f*fa-fa;             snprintf(unit,12,"Нм");     break;
    case 53: *v = (fb-128.0f)*1.4222f+0.006f*fa; snprintf(unit,12,"г/с"); break;
    case 54: *v = fa*256.0f+fb;               snprintf(unit,12,"шт");     break;
    case 55: *v = fa*fb/200.0f;               snprintf(unit,12,"с");      break;
    case 56: *v = fa*256.0f+fb;               snprintf(unit,12,"WSC");    break;
    case 57: *v = fa*256.0f+fb+65536.0f;      snprintf(unit,12,"WSC");    break;
    case 58: *v = (b>128)?1.0225f*(256.0f-fb):1.0225f*fb; snprintf(unit,12,"1/с"); break;
    case 59: *v = (fa*256.0f+fb)/32768.0f;    snprintf(unit,12,"-");      break;
    case 60: *v = (fa*256.0f+fb)*0.01f;       snprintf(unit,12,"с");      break;
    case 61: *v = (a==0)?(fb-128.0f):(fb-128.0f)/fa; snprintf(unit,12,"-"); break;
    case 62: *v = 0.256f*fa*fb;               snprintf(unit,12,"С");      break;
    case 63: *v = 0;                          snprintf(unit,12,"txt");    break;
    case 64: *v = fa+fb;                      snprintf(unit,12,"Ом");     break; /*  */
    case 65: *v = 0.01f*fa*(fb-127.0f);       snprintf(unit,12,"мм");     break;
    case 66: *v = (fa*fb)/511.12f;            snprintf(unit,12,"В");      break;
    case 67: *v = (640.0f*fa)+fb*2.5f;        snprintf(unit,12,"°");      break;
    case 68: *v = (256.0f*fa+fb)/7.365f;      snprintf(unit,12,"°/с");    break;
    case 69: *v = (256.0f*fa+fb)*0.3254f;     snprintf(unit,12,"бар");    break;
    case 70: *v = (256.0f*fa+fb)*0.192f;      snprintf(unit,12,"м/с2");   break;
    default: *v = fa*256.0f+fb;               snprintf(unit,12,"raw");    break;
    }
}

/* ==== LIVE-поток: сессия держится открытой, группы опрашиваются без реконнекта ==== */
static uint8_t s_stream_open = 0;

static void meas_fill(KLineMeas_t *out, uint8_t group, const uint8_t *resp) {
    out->ok = 1;
    out->group = group;
    for (uint8_t ch = 0; ch < 4; ch++) {
        uint8_t t = resp[ch*3], a = resp[1+ch*3], b = resp[2+ch*3];
        out->type[ch] = t;
        out->raw[ch]  = ((uint16_t)a << 8) | b;
        parse_meas_field(t, a, b, &out->val[ch], &out->unit[ch][0]);
    }
}

uint8_t KLine_MeasStream_Start(uint8_t addr) {
    uint8_t k1 = 0, k2 = 0;
    if (KLine_KWP1281_Dialog(addr, &k1, &k2) > 2) return 0;
    char dummy[96]; uint8_t dl = 0;
    KWP1281_ReadIdent(dummy, sizeof(dummy), &dl);   /* слив ident один раз */
    s_stream_open = 1;
    return 1;
}

void KLine_MeasStream_Stop(void) {
    if (s_stream_open) { KWP1281_End(); s_stream_open = 0; }
}

uint8_t KLine_MeasStream_IsOpen(void) { return s_stream_open; }

uint8_t KLine_MeasStream_Poll(uint8_t group, KLineMeas_t *out) {
    if (!s_stream_open) return 0;
    uint8_t resp[32], rlen = 0;
    if (!KWP1281_SendBlock(0x29, &group, 1))        { s_stream_open = 0; return 0; }
    uint8_t t = 0, dl = 0;
    if (!KWP1281_RecvBlock(&t, resp, 32, &dl, 300)) { s_stream_open = 0; return 0; }
    if (t != 0xE7 || dl < 12)                       { s_stream_open = 0; return 0; }
    meas_fill(out, group, resp);
    return 1;
}





uint8_t KLine_KWP1281_DialogM(uint8_t addr, uint8_t mode, uint8_t *kb1, uint8_t *kb2) {
    if (KLine_KWP1281_Connect(addr, mode, kb1, kb2)) return 0;
    HAL_Delay(200);
    if (KLine_KWP1281_Connect(addr, mode, kb1, kb2)) return 0;
    return 3;
}
/* старая обёртка = режим 0 */
uint8_t KLine_KWP1281_Dialog(uint8_t addr, uint8_t *kb1, uint8_t *kb2) {
    return KLine_KWP1281_DialogM(addr, 0, kb1, kb2);
}




uint8_t KLine_GetDtcList(uint8_t addr, KLineDtcList_t *out) {
    uint8_t k1 = 0, k2 = 0;
    out->count = 0;

    if (KLine_KWP1281_Dialog(addr, &k1, &k2) > 2) return 0;

    char dummy[96]; uint8_t dl = 0;
    KWP1281_ReadIdent(dummy, sizeof(dummy), &dl);

    uint8_t resp[48], rlen = 0;
    if (!KWP1281_ReadDTC(resp, &rlen)) { KWP1281_End(); return 0; }
    KWP1281_End();

    // ИСПРАВЛЕНО: данные начинаются с индекса 0
    out->count = 0;
    for (uint8_t i = 0; (i + 3) <= rlen && out->count < KLINE_DTC_MAX; i += 3) {
        uint16_t code = ((uint16_t)resp[i] << 8) | resp[i+1];
        if (code == 0xFFFF || code == 0x0000) continue;

        uint8_t st = resp[i+2];
        KLineDtc_t *d = &out->list[out->count++];
        d->code = code;
        d->type = st;

        char fs[10];
        uint8_t n0 = (code >> 12) & 0xF, n1 = (code >> 8) & 0xF;
        uint8_t n2 = (code >> 4)  & 0xF, n3 = code & 0xF;

        if (n0 < 10 && n1 < 10 && n2 < 10 && n3 < 10) {
            static const char ch[4] = { 'P', 'C', 'B', 'U' };
            snprintf(fs, sizeof(fs), "%c%u%u%u%u", ch[(code >> 14) & 3], n0, n1, n2, n3);
        } else {
            snprintf(fs, sizeof(fs), "%05u", (unsigned)code);
        }

        const char *fl = (st & 0x40) ? " SPOR" : ((st & 0x01) ? " MIL" : "");
        snprintf(d->txt, sizeof(d->txt), "%s%s st=%02X", fs, fl, st);
    }

    return 1;
}

uint8_t KLine_ClearDtcs(uint8_t addr) {
    uint8_t k1 = 0, k2 = 0;
    if (KLine_KWP1281_Dialog(addr, &k1, &k2) > 2) return 0;

    char dummy[96]; uint8_t dl = 0;
    KWP1281_ReadIdent(dummy, sizeof(dummy), &dl);

    uint8_t ok = KWP1281_ClearDTC();
    KWP1281_End();
    return ok;
}
