#include "icons.h"

void Icon_Draw(uint16_t x, uint16_t y, IconId_t id, uint16_t color) {
    switch(id) {
    case ICON_SPEED:
        ILI9481_DrawCircle(x+16, y+16, 13, color);
        ILI9481_DrawLine(x+16, y+16, x+24, y+8, color);
        ILI9481_FillCircle(x+16, y+16, 2, color);
        ILI9481_DrawLine(x+16, y+3,  x+16, y+6,  color);
        ILI9481_DrawLine(x+3,  y+16, x+6,  y+16, color);
        ILI9481_DrawLine(x+26, y+16, x+29, y+16, color);
        break;
    case ICON_RPM:
        ILI9481_DrawCircle(x+16, y+16, 13, color);
        ILI9481_DrawLine(x+16, y+16, x+8, y+8, color);
        ILI9481_FillCircle(x+16, y+16, 2, color);
        ILI9481_DrawLine(x+16, y+3, x+16, y+6, color);
        ILI9481_DrawLine(x+5,  y+6, x+7,  y+9, color);
        ILI9481_DrawLine(x+27, y+6, x+25, y+9, color);
        break;
    case ICON_THERMO:
        ILI9481_DrawRect(x+13, y+3, 6, 19, color);
        ILI9481_FillRect(x+14, y+11, 4, 11, color);
        ILI9481_FillCircle(x+16, y+24, 5, color);
        ILI9481_DrawLine(x+21, y+6,  x+25, y+6,  color);
        ILI9481_DrawLine(x+21, y+11, x+25, y+11, color);
        ILI9481_DrawLine(x+21, y+16, x+25, y+16, color);
        break;
    case ICON_DROP:
        ILI9481_DrawCircle(x+16, y+19, 8, color);
        ILI9481_DrawLine(x+16, y+5, x+9,  y+16, color);
        ILI9481_DrawLine(x+16, y+5, x+23, y+16, color);
        ILI9481_FillCircle(x+16, y+19, 3, color);
        break;
    case ICON_FUEL:
        ILI9481_DrawRect(x+6, y+5, 13, 22, color);
        ILI9481_FillRect(x+9, y+7, 8, 6, color);
        ILI9481_DrawLine(x+19, y+10, x+24, y+10, color);
        ILI9481_DrawLine(x+24, y+10, x+24, y+24, color);
        ILI9481_DrawLine(x+22, y+24, x+26, y+24, color);
        break;
    case ICON_DIST:
        ILI9481_DrawLine(x+6,  y+29, x+13, y+3, color);
        ILI9481_DrawLine(x+26, y+29, x+19, y+3, color);
        ILI9481_DrawLine(x+16, y+6,  x+16, y+9,  color);
        ILI9481_DrawLine(x+16, y+14, x+16, y+17, color);
        ILI9481_DrawLine(x+16, y+22, x+16, y+25, color);
        break;
    case ICON_BATTERY:
        ILI9481_DrawRect(x+5, y+10, 22, 16, color);
        ILI9481_FillRect(x+9,  y+7, 3, 3, color);
        ILI9481_FillRect(x+19, y+7, 3, 3, color);
        ILI9481_DrawLine(x+9,  y+18, x+13, y+18, color);
        ILI9481_DrawLine(x+19, y+18, x+23, y+18, color);
        ILI9481_DrawLine(x+21, y+16, x+21, y+20, color);
        break;
    default: break;
    }
}

/* Произвольная битмапа 32x32, 1bpp (4 байта/строку) */
void Icon_DrawBitmap(uint16_t x, uint16_t y, const uint8_t* bmp, uint16_t color) {
    for(uint8_t row = 0; row < ICON_SIZE; row++) {
        for(uint8_t col = 0; col < ICON_SIZE; col++) {
            uint16_t bit_off = row * ICON_SIZE + col;
            if(bmp[bit_off >> 3] & (0x80 >> (bit_off & 7))) {
                ILI9481_DrawPixel(x + col, y + row, color);
            }
        }
    }
}
