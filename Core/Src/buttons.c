#include "buttons.h"

/* Конфигурация пинов */
typedef struct {
    GPIO_TypeDef* port;
    uint16_t      pin;
} ButtonPin_t;

static const ButtonPin_t btn_pins[BTN_COUNT] = {
    {GPIOC, GPIO_PIN_1},  // BTN_UP
    {GPIOC, GPIO_PIN_2},  // BTN_DOWN
    {GPIOC, GPIO_PIN_3}   // BTN_OK
};

/* Состояния кнопок */
typedef struct {
    ButtonState_t state;
    uint32_t      press_time;
    uint32_t      press_count;
    uint8_t       event_flag;
    uint8_t       last_raw;
} ButtonData_t;

static ButtonData_t btn_data[BTN_COUNT] = {0};

void Buttons_Init(void)
{
    for (uint8_t i = 0; i < BTN_COUNT; i++) {
        btn_data[i].state = BTN_STATE_RELEASED;
        btn_data[i].press_time = 0;
        btn_data[i].press_count = 0;
        btn_data[i].event_flag = 0;
        btn_data[i].last_raw = 1; // Pull-up = 1
    }
}

void Buttons_Poll(void)
{
    uint32_t now = HAL_GetTick();

    for (uint8_t i = 0; i < BTN_COUNT; i++) {
        uint8_t raw = HAL_GPIO_ReadPin(btn_pins[i].port, btn_pins[i].pin);

        // Детектирование нажатия (1 -> 0)
        if (raw == GPIO_PIN_RESET && btn_data[i].last_raw == GPIO_PIN_SET) {
            btn_data[i].press_time = now;
            btn_data[i].state = BTN_STATE_PRESSED;
            btn_data[i].event_flag = 1;
            btn_data[i].press_count++;
        }

        // Детектирование длительного нажатия
        if (btn_data[i].state == BTN_STATE_PRESSED &&
            (now - btn_data[i].press_time >= BTN_LONG_PRESS_MS)) {
            btn_data[i].state = BTN_STATE_LONG_PRESS;
        }

        // Отпускание (0 -> 1)
        if (raw == GPIO_PIN_SET && btn_data[i].state != BTN_STATE_RELEASED) {
            btn_data[i].state = BTN_STATE_RELEASED;
        }

        btn_data[i].last_raw = raw;
    }
}

ButtonState_t Buttons_GetState(ButtonID_t btn)
{
    if (btn == BTN_NONE || btn >= BTN_COUNT) return BTN_STATE_RELEASED;
    return btn_data[btn - 1].state;
}

uint8_t Buttons_GetEvent(ButtonID_t btn)
{
    if (btn == BTN_NONE || btn >= BTN_COUNT) return 0;

    uint8_t event = btn_data[btn - 1].event_flag;
    btn_data[btn - 1].event_flag = 0; // Сброс флага после чтения
    return event;
}

uint32_t Buttons_GetPressCount(ButtonID_t btn)
{
    if (btn == BTN_NONE || btn >= BTN_COUNT) return 0;
    return btn_data[btn - 1].press_count;
}

uint8_t Buttons_AnyPressed(void)
{
    for (uint8_t i = 0; i < BTN_COUNT; i++) {
        if (btn_data[i].state != BTN_STATE_RELEASED) {
            return 1;
        }
    }
    return 0;
}
