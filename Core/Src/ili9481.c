/* USER CODE BEGIN Header */
/*
  @file    ili9481.c
  @brief   ILI9481 TFT LCD driver with Frame Buffer
*/
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "ili9481.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
   Frame Buffer (D1 SRAM)
   ============================================================================ */
__attribute__((section(".ram_d1"), aligned(32)))
uint16_t frame_buffer[ILI9481_PIXELS];

/* ============================================================================
   Low-level functions
   ============================================================================ */
static inline void ILI9481_WriteCmd(uint8_t cmd)
{
    *(__IO uint16_t *)LCD_CMD_ADDR = cmd;
}

static inline void ILI9481_WriteData(uint16_t data)
{
    *(__IO uint16_t *)LCD_DATA_ADDR = data;
}

static inline void ILI9481_WriteData8(uint8_t data)
{
    *(__IO uint16_t *)LCD_DATA_ADDR = data;
}

static void ILI9481_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    ILI9481_WriteCmd(0x2A);
    ILI9481_WriteData(x1 >> 8);
    ILI9481_WriteData(x1 & 0xFF);
    ILI9481_WriteData(x2 >> 8);
    ILI9481_WriteData(x2 & 0xFF);

    ILI9481_WriteCmd(0x2B);
    ILI9481_WriteData(y1 >> 8);
    ILI9481_WriteData(y1 & 0xFF);
    ILI9481_WriteData(y2 >> 8);
    ILI9481_WriteData(y2 & 0xFF);

    ILI9481_WriteCmd(0x2C);
}

/* ============================================================================
   Initialization
   ============================================================================ */
void ILI9481_Init(void)
{
    HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_RESET);
    ILI9481_Delay(50);
    HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_SET);
    ILI9481_Delay(150);

    ILI9481_WriteCmd(0x01); ILI9481_Delay(120);
    ILI9481_WriteCmd(0x11); ILI9481_Delay(120);

    ILI9481_WriteCmd(0xD0);
    ILI9481_WriteData8(0x07); ILI9481_WriteData8(0x42); ILI9481_WriteData8(0x1C);

    ILI9481_WriteCmd(0xD1);
    ILI9481_WriteData8(0x00); ILI9481_WriteData8(0x1C); ILI9481_WriteData8(0x1F);

    ILI9481_WriteCmd(0xD2);
    ILI9481_WriteData8(0x01); ILI9481_WriteData8(0x11);

    ILI9481_WriteCmd(0xC0);
    ILI9481_WriteData8(0x10); ILI9481_WriteData8(0x3B); ILI9481_WriteData8(0x00);
    ILI9481_WriteData8(0x02); ILI9481_WriteData8(0x11);

    ILI9481_WriteCmd(0xC6); ILI9481_WriteData8(0x83);
    ILI9481_WriteCmd(0xB0); ILI9481_WriteData8(0x00);

    ILI9481_WriteCmd(0x36); ILI9481_WriteData8(ILI9481_ROT_0);
    ILI9481_WriteCmd(0x3A); ILI9481_WriteData8(0x55);
    ILI9481_WriteCmd(0x29); ILI9481_Delay(50);
    ILI9481_WriteCmd(0x2C);
}

void ILI9481_InitBuffer(void)
{
    ILI9481_Clear(COLOR_BLACK);
    ILI9481_FlushBuffer();
}

/* ============================================================================
   Flush Buffer
   ============================================================================ */
void ILI9481_FlushBuffer(void)
{
    // 1. Выталкиваем кэш D1 SRAM в память (обязательно для M7)
    SCB_CleanDCache_by_Addr((uint32_t *)frame_buffer, ILI9481_BUFFER_SIZE);
    __DSB(); __ISB();

    // 2. Устанавливаем окно на весь экран
    ILI9481_SetWindow(0, 0, ILI9481_WIDTH - 1, ILI9481_HEIGHT - 1);

    // 3. Тактирование DMA2D
    __HAL_RCC_DMA2D_CLK_ENABLE();

    // 4. Настройка Chrom-ART (M2M, RGB565  RGB565)
    DMA2D->CR      = DMA2D_M2M;                  // Режим Memory-to-Memory
    DMA2D->OPFCCR  = DMA2D_OUTPUT_RGB565;        // Выходной формат
    DMA2D->OMAR    = (uint32_t)LCD_DATA_ADDR;    // Куда писать (FMC Data)
    DMA2D->OOR     = 0;                          // Смещение строк выхода

    DMA2D->FGPFCCR = DMA2D_INPUT_RGB565;         // Входной формат
    DMA2D->FGMAR   = (uint32_t)frame_buffer;     // Откуда читать (D1 SRAM)
    DMA2D->FGOR    = 0;                          // Смещение строк входа

    DMA2D->NLR     = (ILI9481_HEIGHT << 16) | ILI9481_WIDTH; // Высота | Ширина

    // 5. Сброс флагов ошибок (0x0F = очистка битов 0-3)
    DMA2D->IFCR    = 0x0F;

    // 6. Запуск и ожидание
    DMA2D->CR |= DMA2D_CR_START;
    while(DMA2D->CR & DMA2D_CR_START) {
        __WFE(); // Ядро спит, DMA2D работает
    }
}
/* ============================================================================
   Primitives
   ============================================================================ */
void ILI9481_Clear(uint16_t color)
{
    uint32_t color32 = ((uint32_t)color << 16) | color;
    uint32_t *buf32 = (uint32_t *)frame_buffer;
    uint32_t total_words = ILI9481_PIXELS / 2;
    for(uint32_t i = 0; i < total_words; i++) buf32[i] = color32;
    if(ILI9481_PIXELS & 1) ((uint16_t *)frame_buffer)[ILI9481_PIXELS - 1] = color;
}

void ILI9481_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if(x >= ILI9481_WIDTH || y >= ILI9481_HEIGHT) return;
    frame_buffer[y * ILI9481_WIDTH + x] = color;
}

void ILI9481_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if(x >= ILI9481_WIDTH || y >= ILI9481_HEIGHT) return;
    if(x + w > ILI9481_WIDTH) w = ILI9481_WIDTH - x;
    if(y + h > ILI9481_HEIGHT) h = ILI9481_HEIGHT - y;
    for(uint16_t row = 0; row < h; row++) {
        uint16_t *line = &frame_buffer[(y + row) * ILI9481_WIDTH + x];
        for(uint16_t col = 0; col < w; col++) line[col] = color;
    }
}

void ILI9481_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    ILI9481_DrawHLine(x, y, w, color);
    ILI9481_DrawHLine(x, y + h - 1, w, color);
    ILI9481_DrawVLine(x, y, h, color);
    ILI9481_DrawVLine(x + w - 1, y, h, color);
}

void ILI9481_DrawHLine(uint16_t x, uint16_t y, uint16_t len, uint16_t color)
{
    ILI9481_FillRect(x, y, len, 1, color);
}

void ILI9481_DrawVLine(uint16_t x, uint16_t y, uint16_t len, uint16_t color)
{
    ILI9481_FillRect(x, y, 1, len, color);
}

void ILI9481_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    int16_t dx = abs(x1 - x0);
    int16_t dy = abs(y1 - y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;
    while(1) {
        ILI9481_DrawPixel(x0, y0, color);
        if(x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if(e2 > -dy) { err -= dy; x0 += sx; }
        if(e2 < dx) { err += dx; y0 += sy; }
    }
}

void ILI9481_DrawCircle(int16_t x0, int16_t y0, uint16_t r, uint16_t color)
{
    int16_t x = r, y = 0, err = 0;
    while(x >= y) {
        ILI9481_DrawPixel(x0 + x, y0 + y, color);
        ILI9481_DrawPixel(x0 + y, y0 + x, color);
        ILI9481_DrawPixel(x0 - y, y0 + x, color);
        ILI9481_DrawPixel(x0 - x, y0 + y, color);
        ILI9481_DrawPixel(x0 - x, y0 - y, color);
        ILI9481_DrawPixel(x0 - y, y0 - x, color);
        ILI9481_DrawPixel(x0 + y, y0 - x, color);
        ILI9481_DrawPixel(x0 + x, y0 - y, color);
        if(err <= 0) { y++; err += 2 * y + 1; }
        if(err > 0) { x--; err -= 2 * x + 1; }
    }
}

void ILI9481_FillCircle(int16_t x0, int16_t y0, uint16_t r, uint16_t color)
{
    int16_t x = r, y = 0, err = 0;
    while(x >= y) {
        ILI9481_DrawHLine(x0 - x, y0 + y, 2 * x + 1, color);
        ILI9481_DrawHLine(x0 - x, y0 - y, 2 * x + 1, color);
        ILI9481_DrawHLine(x0 - y, y0 + x, 2 * y + 1, color);
        ILI9481_DrawHLine(x0 - y, y0 - x, 2 * y + 1, color);
        if(err <= 0) { y++; err += 2 * y + 1; }
        if(err > 0) { x--; err -= 2 * x + 1; }
    }
}

void ILI9481_HGradient(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                       uint16_t color1, uint16_t color2)
{
    for(uint16_t i = 0; i < w; i++) {
        uint8_t r = ((color1 >> 11) * (w - i) + (color2 >> 11) * i) / w;
        uint8_t g = (((color1 >> 5) & 0x3F) * (w - i) + ((color2 >> 5) & 0x3F) * i) / w;
        uint8_t b = ((color1 & 0x1F) * (w - i) + (color2 & 0x1F) * i) / w;
        uint16_t col = (r << 11) | (g << 5) | b;
        ILI9481_DrawVLine(x + i, y, h, col);
    }
}

void ILI9481_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
    if(x >= ILI9481_WIDTH || y >= ILI9481_HEIGHT) return;
    if(x + w > ILI9481_WIDTH) w = ILI9481_WIDTH - x;
    if(y + h > ILI9481_HEIGHT) h = ILI9481_HEIGHT - y;
    for(uint16_t row = 0; row < h; row++) {
        const uint16_t *src = &data[row * w];
        uint16_t *dst = &frame_buffer[(y + row) * ILI9481_WIDTH + x];
        memcpy(dst, src, w * sizeof(uint16_t));
    }
}

void ILI9481_DrawImage_QSPI(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t image_addr)
{
    if (x >= ILI9481_WIDTH || y >= ILI9481_HEIGHT) return;
    if (x + w > ILI9481_WIDTH) w = ILI9481_WIDTH - x;
    if (y + h > ILI9481_HEIGHT) h = ILI9481_HEIGHT - y;

    // QSPI кэшируется  инвалидируем, чтобы CPU прочитал свежие данные
    uint32_t byte_len = (uint32_t)w * h * 2;
    SCB_InvalidateDCache_by_Addr((uint32_t*)image_addr, byte_len);
    __DSB(); __ISB();

    volatile const uint16_t *src = (volatile const uint16_t *)image_addr;
    uint16_t *dst_line = &frame_buffer[y * ILI9481_WIDTH + x];

    for (uint16_t row = 0; row < h; row++) {
        // Копируем строку целиком (memcpy быстрее построчного цикла)
        memcpy(dst_line, (void*)src, w * sizeof(uint16_t));
        dst_line += ILI9481_WIDTH;
        src += w;
    }
}
/* ============================================================================
   FONT 5x7 - COMPLETE ASCII + CYRILLIC (Windows-1251)
   ============================================================================ */
static const uint8_t Font5x7[] = {
    /* ASCII 32-127 (96 characters, 8 bytes each) */
		0x00,0x00,0x00,0x00,0x00,   // (space)
						0x00,0x00,0x5F,0x00,0x00,   // !
						0x00,0x07,0x00,0x07,0x00,   // "
						0x14,0x7F,0x14,0x7F,0x14,   // #
						0x24,0x2A,0x7F,0x2A,0x12,   // $
						0x23,0x13,0x08,0x64,0x62,   // %
						0x36,0x49,0x55,0x22,0x50,   // &
						0x00,0x05,0x03,0x00,0x00,   // '
						0x00,0x1C,0x22,0x41,0x00,   // (
						0x00,0x41,0x22,0x1C,0x00,   // )
						0x08,0x2A,0x1C,0x2A,0x08,   // *
						0x08,0x08,0x3E,0x08,0x08,   // +
						0x00,0x50,0x30,0x00,0x00,   // ,
						0x08,0x08,0x08,0x08,0x08,   // -
						0x00,0x60,0x60,0x00,0x00,   // .
						0x20,0x10,0x08,0x04,0x02,   // /
						0x3E,0x51,0x49,0x45,0x3E,   // 0
						0x00,0x42,0x7F,0x40,0x00,   // 1
						0x42,0x61,0x51,0x49,0x46,   // 2
						0x21,0x41,0x45,0x4B,0x31,   // 3
						0x18,0x14,0x12,0x7F,0x10,   // 4
						0x27,0x45,0x45,0x45,0x39,   // 5
						0x3C,0x4A,0x49,0x49,0x30,   // 6
						0x01,0x71,0x09,0x05,0x03,   // 7
						0x36,0x49,0x49,0x49,0x36,   // 8
						0x06,0x49,0x49,0x29,0x1E,   // 9
						0x00,0x36,0x36,0x00,0x00,   // :
						0x00,0x56,0x36,0x00,0x00,   // ;
						0x00,0x08,0x14,0x22,0x41,   // <
						0x14,0x14,0x14,0x14,0x14,   // =
						0x41,0x22,0x14,0x08,0x00,   // >
						0x02,0x01,0x51,0x09,0x06,   // ?
						0x32,0x49,0x79,0x41,0x3E,   // @
						0x7E,0x11,0x11,0x11,0x7E,   // A
						0x7F,0x49,0x49,0x49,0x36,   // B
						0x3E,0x41,0x41,0x41,0x22,   // C
						0x7F,0x41,0x41,0x22,0x1C,   // D
						0x7F,0x49,0x49,0x49,0x41,   // E
						0x7F,0x09,0x09,0x01,0x01,   // F
						0x3E,0x41,0x41,0x51,0x32,   // G
						0x7F,0x08,0x08,0x08,0x7F,   // H
						0x00,0x41,0x7F,0x41,0x00,   // I
						0x20,0x40,0x41,0x3F,0x01,   // J
						0x7F,0x08,0x14,0x22,0x41,   // K
						0x7F,0x40,0x40,0x40,0x40,   // L
						0x7F,0x02,0x04,0x02,0x7F,   // M
						0x7F,0x04,0x08,0x10,0x7F,   // N
						0x3E,0x41,0x41,0x41,0x3E,   // O
						0x7F,0x09,0x09,0x09,0x06,   // P
						0x3E,0x41,0x51,0x21,0x5E,   // Q
						0x7F,0x09,0x19,0x29,0x46,   // R
						0x46,0x49,0x49,0x49,0x31,   // S
						0x01,0x01,0x7F,0x01,0x01,   // T
						0x3F,0x40,0x40,0x40,0x3F,   // U
						0x1F,0x20,0x40,0x20,0x1F,   // V
						0x7F,0x20,0x18,0x20,0x7F,   // W
						0x63,0x14,0x08,0x14,0x63,   // X
						0x03,0x04,0x78,0x04,0x03,   // Y
						0x61,0x51,0x49,0x45,0x43,   // Z
						0x00,0x00,0x7F,0x41,0x41,   // [
						0x02,0x04,0x08,0x10,0x20,   // "\"
						0x41,0x41,0x7F,0x00,0x00,   // ]
						0x04,0x02,0x01,0x02,0x04,   // ^
						0x40,0x40,0x40,0x40,0x40,   // _
						0x00,0x01,0x02,0x04,0x00,   // `
						0x20,0x54,0x54,0x54,0x78,   // a
						0x7F,0x48,0x44,0x44,0x38,   // b
						0x38,0x44,0x44,0x44,0x20,   // c
						0x38,0x44,0x44,0x48,0x7F,   // d
						0x38,0x54,0x54,0x54,0x18,   // e
						0x08,0x7E,0x09,0x01,0x02,   // f
						0x08,0x14,0x54,0x54,0x3C,   // g
						0x7F,0x08,0x04,0x04,0x78,   // h
						0x00,0x44,0x7D,0x40,0x00,   // i
						0x20,0x40,0x44,0x3D,0x00,   // j
						0x00,0x7F,0x10,0x28,0x44,   // k
						0x00,0x41,0x7F,0x40,0x00,   // l
						0x7C,0x04,0x18,0x04,0x78,   // m
						0x7C,0x08,0x04,0x04,0x78,   // n
						0x38,0x44,0x44,0x44,0x38,   // o
						0x7C,0x14,0x14,0x14,0x08,   // p
						0x08,0x14,0x14,0x18,0x7C,   // q
						0x7C,0x08,0x04,0x04,0x08,   // r
						0x48,0x54,0x54,0x54,0x20,   // s
						0x04,0x3F,0x44,0x40,0x20,   // t
						0x3C,0x40,0x40,0x20,0x7C,   // u
						0x1C,0x20,0x40,0x20,0x1C,   // v
						0x3C,0x40,0x30,0x40,0x3C,   // w
						0x44,0x28,0x10,0x28,0x44,   // x
						0x0C,0x50,0x50,0x50,0x3C,   // y
						0x44,0x64,0x54,0x4C,0x44,   // z
						0x00,0x08,0x36,0x41,0x00,   // {
						0x00,0x00,0x7F,0x00,0x00,   // |
						0x00,0x41,0x36,0x08,0x00,   // }
						0x55,0xAA,0x55,0xAA,0x55    // unknown char (code 0x7E)
};


void ILI9481_DrawChar(uint16_t x, uint16_t y, char c,
                      uint16_t color, uint16_t bg, uint8_t size)
{

    if(c < 32 || c > 126) c = 32;


    const uint8_t *glyph = &Font5x7[(c - 32) * 5];


    for(uint8_t col = 0; col < 5; col++) {
        uint8_t column_data = glyph[col];


        for(uint8_t row = 0; row < 7; row++) {
            uint8_t pixel = (column_data >> row) & 1;
            uint16_t px_color = pixel ? color : bg;


            for(uint8_t sy = 0; sy < size; sy++) {
                for(uint8_t sx = 0; sx < size; sx++) {
                    ILI9481_DrawPixel(x + col * size + sx, y + row * size + sy, px_color);
                }
            }
        }
    }
}

/* ============================================================================
   Draw String
   ============================================================================ */
void ILI9481_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size)
{
    uint16_t cursor_x = x;
    while(*str) {
        if(*str == '\n') {
            cursor_x = x;
            y += 7 * size + 1;
        } else if(*str != '\r') {
            ILI9481_DrawChar(cursor_x, y, *str, color, bg, size);
            cursor_x += 5 * size + 1;
        }
        str++;
    }
}

/* ============================================================================
   Draw UTF-8 String (convert to Windows-1251)
   ============================================================================ */
void ILI9481_DrawString_UTF8(uint16_t x, uint16_t y, const char *str,
                             uint16_t color, uint16_t bg, uint8_t size)
{

    ILI9481_DrawString(x, y, str, color, bg, size);
}



static inline uint8_t QSPI_ReadByte(uint32_t addr)
{
    volatile uint8_t *p = (volatile uint8_t *)addr;
    return *p;
}

static inline uint8_t GetPixelLevel(uint8_t data, uint8_t index)
{
    return (data >> (6 - index * 2)) & 0x03;
}

static uint16_t BlendColor(uint16_t color, uint16_t bg, uint8_t level)
{
    if(level == 0) return bg;
    if(level == 3) return color;

    uint8_t r1 = (color >> 11) & 0x1F;
    uint8_t g1 = (color >> 5) & 0x3F;
    uint8_t b1 = color & 0x1F;

    uint8_t r2 = (bg >> 11) & 0x1F;
    uint8_t g2 = (bg >> 5) & 0x3F;
    uint8_t b2 = bg & 0x1F;

    uint8_t r = (r2 * (3 - level) + r1 * level) / 3;
    uint8_t g = (g2 * (3 - level) + g1 * level) / 3;
    uint8_t b = (b2 * (3 - level) + b1 * level) / 3;

    return (r << 11) | (g << 5) | b;
}


/* ============================================================================
Отрисовка символа из QSPI (1bpp, Unicode индексы)
Конвертер: ASCII 0x20-0x7E, Кириллица 0x400-0x44F, Ё/ё 0x401/0x451
Исходный код: CP1251
============================================================================ */
void ILI9481_DrawChar_QSPI(uint16_t x, uint16_t y, char c,
                           uint16_t color, uint16_t bg,
                           uint32_t font_addr,
                           uint8_t char_width, uint8_t char_height)
{


	    uint8_t char_index;


    if(c >= 32 && c <= 126) {
        char_index = c - 32;
    }
    else if (c == 0xA8) {
            char_index = 175;                         // Ё
        }
        else if (c == 0xB8) {
            char_index = 176;                         // ё
        }
        else if (c == 0xB0) {
            char_index = 177;                         // ° Degree
        }

    else if(c >= 192 && c <= 255) {
        uint32_t unicode = 0;

        if(c >= 0xC0 && c <= 0xDF) {

            unicode = 0x410 + (c - 0xC0);
        }
        else if(c >= 0xE0 && c <= 0xFF) {

            unicode = 0x430 + (c - 0xE0);
        }
        else if(c == 0xA8) {

            unicode = 0x401;
        }
        else if(c == 0xB8) {

            unicode = 0x451;
        }

        /* Вычисляем индекс в шрифте */
        if(unicode >= 0x400 && unicode <= 0x44F) {
            char_index = 95 + (unicode - 0x400);
        }
        else if(unicode == 0x401) {
            char_index = 175;  /* После 0x44F */
        }
        else if(unicode == 0x451) {
            char_index = 176;  /* После Ё */
        }
        else if(unicode == 0xB0) {
            /* Индекс 177:
               95 (ASCII) + 80 (кириллица 0x400-0x44F) + 1 (Ё) + 1 (ё) = 177 */
        	char_index = 176;
        }
        else {
            ILI9481_FillRect(x, y, char_width, char_height, bg);
            return;
        }
    }
    else {
        ILI9481_FillRect(x, y, char_width, char_height, bg);
        return;
    }

    /* 1bpp: 8 пикселей = 1 байт */
    uint32_t bytes_per_row = (char_width + 7) / 8;
    uint32_t bytes_per_char = bytes_per_row * char_height;
    uint32_t char_addr = font_addr + (char_index * bytes_per_char);

    for(uint8_t row = 0; row < char_height; row++) {
        for(uint8_t col = 0; col < char_width; col += 8) {
            uint32_t byte_offset = row * bytes_per_row + (col / 8);
            volatile uint8_t *p = (volatile uint8_t *)(char_addr + byte_offset);
            uint8_t pixel_data = *p;

            for(uint8_t p_idx = 0; p_idx < 8; p_idx++) {
                if(col + p_idx >= char_width) break;

                uint8_t pixel = (pixel_data >> (7 - p_idx)) & 1;

                if (pixel) {
                    // Рисуем символ всегда
                    ILI9481_DrawPixel(x + col + p_idx, y + row, color);
                } else {
                    // Рисуем фон ТОЛЬКО если он НЕ прозрачный
                    if (bg != COLOR_TRANSPARENT) {
                        ILI9481_DrawPixel(x + col + p_idx, y + row, bg);
                    }
                }
            }
        }
    }
}


void ILI9481_DrawString_QSPI(uint16_t x, uint16_t y, const char *str,
                             uint16_t color, uint16_t bg,
                             uint32_t font_addr,
                             uint8_t char_width, uint8_t char_height)
{
    uint16_t cursor_x = x;

    while(*str) {
        if(*str == '\n') {
            cursor_x = x;
            y += char_height +2;
        } else if(*str != '\r') {
            ILI9481_DrawChar_QSPI(cursor_x, y, *str, color, bg,
                                  font_addr, char_width, char_height);
            cursor_x += char_width + (char_width >= 40 ? 1 : 0);
        }
        str++;
    }
}

/* Индекс глифа (та же логика, что в DrawChar_QSPI) */
static uint8_t QSPI_GetCharIndex(char c, uint8_t* out) {
    if(c >= 32 && c <= 126) { *out = c - 32; return 1; }
    if(c == (char)0xA8) { *out = 175; return 1; }
    if(c == (char)0xB8) { *out = 176; return 1; }
    if(c == (char)0xB0) { *out = 177; return 1; }
    if((uint8_t)c >= 192) {
        uint32_t unicode = ((uint8_t)c < 0xE0) ? 0x410 + ((uint8_t)c - 0xC0)
                                               : 0x430 + ((uint8_t)c - 0xE0);
        if(unicode <= 0x44F) { *out = 95 + (unicode - 0x400); return 1; }
    }
    return 0;
}

/* Реальная ширина глифа: сканируем битмап, ищем правый засвеченный пиксель */
static uint8_t QSPI_GlyphWidth(uint32_t font_addr, uint8_t idx,
                               uint8_t char_width, uint8_t char_height) {
    uint32_t bpr = (char_width + 7) / 8;
    uint32_t base = font_addr + (uint32_t)idx * bpr * char_height;
    uint8_t width = 0;
    for(uint8_t row = 0; row < char_height; row++) {
        for(uint8_t col = 0; col < char_width; col++) {
            volatile uint8_t *p = (volatile uint8_t*)(base + row * bpr + (col >> 3));
            if((*p >> (7 - (col & 7))) & 1) { if(col + 1 > width) width = col + 1; }
        }
    }
    return width ? width : (char_width + 1) / 2;   // пробел = половина клетки
}

/* Пропорциональная отрисовка: следующий символ сразу за реальным краем + gap */
void ILI9481_DrawString_QSPI_Pro(uint16_t x, uint16_t y, const char *str,
    uint16_t color, uint16_t bg, uint32_t font_addr,
    uint8_t char_width, uint8_t char_height, uint8_t gap) {
    uint16_t cursor_x = x;
    while(*str) {
        if(*str == '\n') { cursor_x = x; y += char_height + 2; }
        else if(*str != '\r') {
            uint8_t idx;
            if(QSPI_GetCharIndex(*str, &idx)) {
                ILI9481_DrawChar_QSPI(cursor_x, y, *str, color, bg, font_addr, char_width, char_height);
                cursor_x += QSPI_GlyphWidth(font_addr, idx, char_width, char_height) + gap;
            } else cursor_x += (char_width + 1) / 2;
        }
        str++;
    }
}

/* Ширина строки для выравнивания */
uint16_t ILI9481_StringWidth_QSPI_Pro(const char *str, uint32_t font_addr,
    uint8_t char_width, uint8_t char_height, uint8_t gap) {
    uint16_t w = 0;
    while(*str) {
        uint8_t idx;
        w += (QSPI_GetCharIndex(*str, &idx))
             ? QSPI_GlyphWidth(font_addr, idx, char_width, char_height) + gap
             : (char_width + 1) / 2;
        str++;
    }
    return w ? w - gap : 0;
}

