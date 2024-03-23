#include <stdint.h>
#include <stdbool.h>

#include "common.h"
#include "joypad.h"

Joypad *
joypad_new(void)
{
    Joypad *joypad = xalloc(sizeof(Joypad));
    joypad_reset(joypad);
    return joypad;
}

void
joypad_free(Joypad **joypad)
{
    xfree(*joypad);
}

void
joypad_reset(Joypad *joypad)
{
    joypad->select_dpad = false;
    joypad->select_btn = false;
    joypad->buttons = 0;
    joypad->dpad = 0;
}

uint8_t
joypad_read(Joypad *joypad)
{
    uint8_t data = 0x00;

    if (joypad->select_dpad) {
        data |= joypad->dpad;
    }

    if (joypad->select_btn) {
        data |= joypad->buttons;
    }

    return data;
}

void
joypad_write(Joypad *joypad, uint8_t data)
{
    joypad->select_dpad = (data & 0x10) == 0;
    joypad->select_btn = (data & 0x20) == 0;
}

void inline
joypad_clear(Joypad *joypad)
{
    joypad->dpad = 0x0F;
    joypad->buttons = 0x0F;
}

void inline
joypad_press(Joypad *joypad, JoypadButton button)
{
    switch (button) {
    case JOYPAD_RIGHT: joypad->dpad &= ~0x01; break;
    case JOYPAD_LEFT: joypad->dpad &= ~0x02; break;
    case JOYPAD_UP: joypad->dpad &= ~0x04; break;
    case JOYPAD_DOWN: joypad->dpad &= ~0x08; break;
    case JOYPAD_A: joypad->buttons &= ~0x01; break;
    case JOYPAD_B: joypad->buttons &= ~0x02; break;
    case JOYPAD_SELECT: joypad->buttons &= ~0x04; break;
    case JOYPAD_START: joypad->buttons &= ~0x08; break;
    default: PANIC("invalid button: %d", button);
    }
}
