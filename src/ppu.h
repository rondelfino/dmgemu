#pragma once

#include "common.h"

typedef struct
{
    struct color
    {
        u8 r, g, b;
    } colors[5];
} GBPalette;

extern const GBPalette PALETTE_GREY;
extern const GBPalette PALETTE_DMG;

typedef struct
{
    GBPalette palette;
} LCD;

typedef struct
{

} PPU;

void lcd_set_palette(LCD *lcd, const GBPalette *palette);
