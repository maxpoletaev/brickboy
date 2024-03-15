#include <stdint.h>
#include <stdbool.h>

#include "common.h"
#include "raylib.h"
#include "joypad.h"
#include "ui.h"

const Color ui_palettes[][4] = {
    { // GREEN
        {181, 198, 156, 255},
        {141, 156, 123, 255},
        {99, 114, 81, 255},
        {48, 56, 32, 255},
    },
    { // ORANGE
        {249, 234, 139, 255},
        {229, 148, 54, 255},
        {150, 66, 32, 255},
        {45, 19, 9, 255},
    },
    { // PINK
        {252, 200, 155, 255},
        {255, 95, 162, 255},
        {150, 55, 95, 255},
        {60, 22, 38, 255},
    },
};

static struct {
    Color frame_pixels[144*160];
    RenderTexture2D frame_texture;
    Color tileset_pixels[128*192];
    RenderTexture2D tileset_texture;
    int palette;
} ui;

void
ui_init(void)
{
    SetTargetFPS(60);
    SetTraceLogLevel(LOG_ERROR);
    InitWindow(UI_WINDOW_WIDTH, UI_WINDOW_HEIGHT, "BrickBoy");

    ui.frame_texture = LoadRenderTexture(160, 144);
    ui.tileset_texture = LoadRenderTexture(128, 192);
}

void
ui_close(void)
{
    UnloadRenderTexture(ui.frame_texture);
    UnloadRenderTexture(ui.tileset_texture);
    CloseWindow();
}

void
ui_update_frame_view(const uint8_t *frame)
{
    BeginTextureMode(ui.frame_texture);

    for (int y = 0; y < 144; y++) {
        for (int x = 0; x < 160; x++) {
            uint8_t pixel = frame[y*160 + x];
            ui.frame_pixels[y*160 + x] = ui_palettes[ui.palette][pixel];
        }
    }

    UpdateTexture(ui.frame_texture.texture, ui.frame_pixels);
    EndTextureMode();
}

static void
ui_draw_tile(const uint8_t *vram, int tile_num, int pos_x, int pos_y, Color *pixels)
{
    uint8_t tile[8][8];

    for (int y = 0; y < 8; y++) {
        uint16_t offset = tile_num*16 + y*2;

        uint8_t b0 = vram[offset + 0];
        uint8_t b1 = vram[offset + 1];

        for (int x = 0; x < 8; x++) {
            uint8_t px = ((b0 >> x) & 0x1) << 1;
            px |= ((b1 >> x) & 0x1) << 0;
            tile[y][7-x] = px;
        }
    }

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            Color color = ui_palettes[ui.palette][tile[y][x]];
            pixels[(pos_y + y) * 128 + (pos_x + x)] = color;
        }
    }
}

void
ui_update_debug_view(const uint8_t *vram)
{
    BeginTextureMode(ui.tileset_texture);

    for (int tile_num  = 0; tile_num < 384; tile_num++) {
        int tile_x = (tile_num % 16) * 8;
        int tile_y = (tile_num / 16) * 8;
        ui_draw_tile(vram, tile_num, tile_x, tile_y, ui.tileset_pixels);
    }

    UpdateTexture(ui.tileset_texture.texture, ui.tileset_pixels);
    DrawLine(0, 0, 0, 192, BLACK);
    DrawLine(0, 0, 144, 1, BLACK);

    EndTextureMode();
}

static void
ui_draw_overlay(void)
{
    int fps = GetFPS();
    DrawText(strfmt("%d fps", fps), 3, 3, 10, BLACK);
    DrawText(strfmt("%d fps", fps), 2, 2, 10, WHITE);
}

static inline void
ui_handle_hotkeys(void)
{
    if (IsKeyPressed(KEY_ONE)) {
        ui.palette = 0;
    } else if (IsKeyPressed(KEY_TWO)) {
        ui.palette = 1;
    } else if (IsKeyPressed(KEY_THREE)) {
        ui.palette = 2;
    }
}

void
ui_refresh(void)
{

    BeginDrawing();
    ClearBackground(PURPLE);

    ui_handle_hotkeys();

    static Vector2 origin = (Vector2) {0, 0};

    static Rectangle frame_rect = (Rectangle) {0, 0, 160, 144};
    static Rectangle frame_srect = (Rectangle) {0, 0, 160*UI_SCALE, 144*UI_SCALE};
    DrawTexturePro(ui.frame_texture.texture, frame_rect, frame_srect, origin, 0, WHITE);

    static Rectangle tileset_rect = (Rectangle) {0, 0, 128, 192};
    static Rectangle tileset_srect = (Rectangle) {160*UI_SCALE, 0, 128*2, 192*2};
    DrawTexturePro(ui.tileset_texture.texture, tileset_rect, tileset_srect, origin, 0, WHITE);

    ui_draw_overlay();
    EndDrawing();
}

static inline bool
ui_modifier_pressed(void)
{
    return IsKeyDown(KEY_LEFT_CONTROL) ||
        IsKeyDown(KEY_RIGHT_CONTROL) ||
        IsKeyDown(KEY_LEFT_SUPER) ||
        IsKeyDown(KEY_RIGHT_SUPER);
}

bool
ui_reset_pressed(void)
{
    return ui_modifier_pressed() && IsKeyPressed(KEY_R);
}

bool
ui_button_pressed(JoypadButton button)
{
    switch (button) {
    case JOYPAD_UP:
        return IsKeyDown(KEY_W);
    case JOYPAD_LEFT:
        return IsKeyDown(KEY_A);
    case JOYPAD_DOWN:
        return IsKeyDown(KEY_S);
    case JOYPAD_RIGHT:
        return IsKeyDown(KEY_D);
    case JOYPAD_A:
        return IsKeyDown(KEY_K);
    case JOYPAD_B:
        return IsKeyDown(KEY_J);
    case JOYPAD_SELECT:
        return IsKeyDown(KEY_RIGHT_SHIFT);
    case JOYPAD_START:
        return IsKeyDown(KEY_ENTER);
    default:
        PANIC("invalid button: %d", button);
    }
}

inline bool
ui_should_pause(void)
{
    return !IsWindowFocused();
}

inline bool
ui_should_close(void)
{
    return WindowShouldClose();
}
