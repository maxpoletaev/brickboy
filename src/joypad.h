#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common.h"

typedef enum {
    JOYPAD_RIGHT = 0,
    JOYPAD_LEFT = 1,
    JOYPAD_UP = 2,
    JOYPAD_DOWN = 3,
    JOYPAD_A = 4,
    JOYPAD_B = 5,
    JOYPAD_SELECT = 6,
    JOYPAD_START = 7,
} JoypadButton;

typedef struct Joypad {
    bool select_dpad;
    bool select_btn;
    uint8_t buttons;
    uint8_t dpad;
} Joypad;

Joypad *joypad_new(void);

void joypad_free(Joypad **joypad);

void joypad_reset(Joypad *joypad);

uint8_t joypad_read(Joypad *joypad);

void joypad_write(Joypad *joypad, uint8_t data);

void joypad_clear(Joypad *joypad);

void joypad_press(Joypad *joypad, JoypadButton button);
