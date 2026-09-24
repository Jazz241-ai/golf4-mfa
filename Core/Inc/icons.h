#ifndef __ICONS_H
#define __ICONS_H

#include "ili9481.h"
#include <stdint.h>

/* Все иконки рисуются в окне 32x32 пикселя */
#define ICON_SIZE 32

typedef enum {
    ICON_SPEED = 0,
    ICON_RPM,
    ICON_THERMO,
    ICON_DROP,
    ICON_FUEL,
    ICON_DIST,
    ICON_BATTERY,
    /*  Новые ID добавлять сюда  */
    ICON_COUNT
} IconId_t;

void Icon_Draw(uint16_t x, uint16_t y, IconId_t id, uint16_t color);
void Icon_DrawBitmap(uint16_t x, uint16_t y, const uint8_t* bmp, uint16_t color);

#endif
