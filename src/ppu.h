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
    int top_left_x;
    int top_left_y;
    int width;
    int height;
} WindowDimension;

typedef struct
{
    u32 *memory;
    u32 width;
    u32 height;
    u32 pitch;
} FrameBuffer;

#define LCD_WIDTH 160
#define LCD_HEIGHT 144

typedef struct
{
    GBPalette palette;
    WindowDimension *dimension;
} LCD;

typedef struct
{
    LCD *lcd;
} PPU;

void lcd_set_palette(LCD *lcd, const GBPalette *palette);
void gb_get_lcd_dimensions(LCD *lcd);
