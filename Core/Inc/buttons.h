#ifndef __BUTTONS_H
#define __BUTTONS_H

#include "main.h"
#include <stdint.h>

/* Идентификаторы кнопок */
typedef enum {
    BTN_NONE = 0,
    BTN_UP,
    BTN_DOWN,
    BTN_OK,
    BTN_COUNT
} ButtonID_t;

/* Состояния кнопки */
typedef enum {
    BTN_STATE_RELEASED = 0,
    BTN_STATE_PRESSED,
    BTN_STATE_LONG_PRESS
} ButtonState_t;

/* Конфигурация */
#define BTN_DEBOUNCE_MS       40      // Время антидребезга
#define BTN_LONG_PRESS_MS     1000    // Длительное нажатие (1 сек)

/* Инициализация */
void Buttons_Init(void);

/* Опрос кнопок (вызывать в цикле) */
void Buttons_Poll(void);

/* Получить состояние кнопки */
ButtonState_t Buttons_GetState(ButtonID_t btn);

/* Получить событие (однократное нажатие) */
uint8_t Buttons_GetEvent(ButtonID_t btn);

/* Получить счётчик нажатий */
uint32_t Buttons_GetPressCount(ButtonID_t btn);

/* Проверка любого нажатия */
uint8_t Buttons_AnyPressed(void);

#endif /* __BUTTONS_H */
