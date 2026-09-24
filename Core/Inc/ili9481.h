/* USER CODE BEGIN Header */
#ifndef __ILI9481_H
#define __ILI9481_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fmc.h"
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>


#define LCD_BASE_ADDR           0x60000000UL
#define LCD_ADDR_LINE_RS        20
#define LCD_CMD_ADDR            (LCD_BASE_ADDR)
#define LCD_DATA_ADDR           (LCD_BASE_ADDR | (1UL << LCD_ADDR_LINE_RS))


#define LCD_RESET_GPIO_Port     GPIOE
#define LCD_RESET_Pin           GPIO_PIN_1
#define LCD_BL_GPIO_Port        GPIOA
#define LCD_BL_Pin              GPIO_PIN_1


#define ILI9481_WIDTH           320
#define ILI9481_HEIGHT          480
#define ILI9481_PIXELS          (ILI9481_WIDTH * ILI9481_HEIGHT)
#define ILI9481_BUFFER_SIZE     (ILI9481_PIXELS * sizeof(uint16_t))


#define ILI9481_ROT_0           0x48
#define ILI9481_ROT_90          0x28
#define ILI9481_ROT_180         0x88
#define ILI9481_ROT_270         0xE8

#define COLOR_TRANSPARENT       0xFFFF
#define COLOR_BLACK             0x0000
#define COLOR_WHITE             0xFFFF
#define COLOR_RED               0xF800
#define COLOR_GREEN             0x07E0
#define COLOR_BLUE              0x001F
#define COLOR_CYAN              0x07FF
#define COLOR_MAGENTA           0xF81F
#define COLOR_YELLOW            0xFFE0
#define COLOR_ORANGE            0xFD20
#define COLOR_PINK              0xF81F
#define COLOR_GRAY              0x8410
#define RGB565(r, g, b)         ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3))


extern uint16_t frame_buffer[ILI9481_PIXELS];


void ILI9481_Init(void);
void ILI9481_InitBuffer(void);
void ILI9481_SetRotation(uint8_t rotation);
void ILI9481_SetBacklight(uint8_t brightness);
void ILI9481_FlushBuffer(void);
void ILI9481_Clear(uint16_t color);
void ILI9481_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
uint16_t ILI9481_GetPixel(uint16_t x, uint16_t y);
void ILI9481_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ILI9481_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ILI9481_DrawHLine(uint16_t x, uint16_t y, uint16_t len, uint16_t color);
void ILI9481_DrawVLine(uint16_t x, uint16_t y, uint16_t len, uint16_t color);
void ILI9481_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void ILI9481_DrawCircle(int16_t x0, int16_t y0, uint16_t r, uint16_t color);
void ILI9481_FillCircle(int16_t x0, int16_t y0, uint16_t r, uint16_t color);
void ILI9481_HGradient(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color1, uint16_t color2);
void ILI9481_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);

void ILI9481_DrawImage_QSPI(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t image_addr);
void ILI9481_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size);
void ILI9481_DrawString_UTF8(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size);
void ILI9481_DrawString_QSPI_Pro(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint32_t font_addr, uint8_t char_width, uint8_t char_height, uint8_t gap);
uint16_t ILI9481_StringWidth_QSPI_Pro(const char *str, uint32_t font_addr, uint8_t char_width, uint8_t char_height, uint8_t gap);

void ILI9481_DrawChar_QSPI(uint16_t x, uint16_t y, char c,
                           uint16_t color, uint16_t bg,
                           uint32_t font_addr, uint8_t char_width, uint8_t char_height);
void ILI9481_DrawString_QSPI(uint16_t x, uint16_t y, const char *str,
                             uint16_t color, uint16_t bg,
                             uint32_t font_addr, uint8_t char_width, uint8_t char_height);

#define ILI9481_Delay(ms) HAL_Delay(ms)

#ifdef __cplusplus
}
#endif
#endif /* __ILI9481_H */
