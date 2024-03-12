#include <stdint.h>
#include <stdbool.h>

#include "common.h"
#include "raylib.h"
#include "ppu.h"
#include "ui.h"

static struct {
    Color frame_pixels[144*160];
    RenderTexture2D frame_texture;
    Color tileset_pixels[128*192];
    RenderTexture2D tileset_texture;
} ui;

void
ui_init(void)
{
    SetTargetFPS(60);
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

static inline Color
ui_to_color(RGB pixel)
{
    return (Color) {pixel.r, pixel.g, pixel.b, 255};
}

void
ui_update_frame_view(const RGB *frame)
{
    BeginTextureMode(ui.frame_texture);

    for (int y = 0; y < 144; y++) {
        for (int x = 0; x < 160; x++) {
            ui.frame_pixels[y*160 + x] = ui_to_color(frame[y*160 + x]);
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
            uint8_t v0 = ((b0 >> x) & 0x1) << 1;
            uint8_t v1 = ((b1 >> x) & 0x1) << 0;
            tile[y][7-x] = v0 | v1;
        }
    }

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            Color color = ui_to_color(ppu_colors[tile[y][x]]);
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

void
ui_refresh(void)
{
    BeginDrawing();
    ClearBackground(PURPLE);

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

bool
ui_should_close(void)
{
    return WindowShouldClose();
}
