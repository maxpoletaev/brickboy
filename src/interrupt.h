#pragma once

typedef enum {
    INT_VBLANK = (1 << 0),
    INT_LCD_STAT = (1 << 1),
    INT_TIMER = (1 << 2),
    INT_SERIAL = (1 << 3),
    INT_JOYPAD = (1 << 4),
} Interrupt;
