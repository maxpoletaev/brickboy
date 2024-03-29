#pragma once

#include <stdbool.h>

#include "raylib.h"
#include "joypad.h"
#include "ppu.h"

#define UI_SCALE 3
#define UI_FPS 60
#define UI_SHOW_DEBUG_VIEW 0
#define UI_WINDOW_HEIGHT (144 * UI_SCALE)
#define UI_WINDOW_WIDTH ((160 * UI_SCALE))
#define UI_DEBUG_VIEW_WIDTH (128 * (UI_SCALE+1) / 2)
#define UI_DEBUG_VIEW_HEIGHT (192 * (UI_SCALE+1) / 2)
#define UI_FILTER TEXTURE_FILTER_POINT

extern const Color ui_palettes[][4];

void ui_init(void);

void ui_update_frame_view(const uint8_t *frame);

void ui_update_debug_view(const uint8_t *vram);

bool ui_button_pressed(JoypadButton button);

bool ui_reset_pressed(void);

void ui_refresh(void);

bool ui_should_pause(void);

bool ui_should_close(void);

void ui_close(void);
