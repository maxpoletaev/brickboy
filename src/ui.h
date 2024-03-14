#pragma once

#include <stdbool.h>

#include "raylib.h"
#include "ppu.h"

#define UI_SCALE 3
#define UI_SHOW_FPS 0
#define UI_WINDOW_HEIGHT (144 * UI_SCALE)
#define UI_WINDOW_WIDTH ((160 * UI_SCALE) + (128 * 2))

void ui_init(void);

void ui_update_frame_view(const RGB *frame);

void ui_update_debug_view(const uint8_t *vram);

void ui_refresh(void);

bool ui_should_close(void);

void ui_close(void);
