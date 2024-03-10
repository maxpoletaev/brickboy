#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common.h"

typedef struct RGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGB;

extern const RGB ppu_colors[4];

typedef enum PPUMode {
    PPU_MODE_HBLANK = 0,
    PPU_MODE_VBLANK = 1,
    PPU_MODE_OAM_SCAN = 2,
    PPU_MODE_PIXEL_DRAW = 3
} PPUMode;

typedef union LCDCRegister {
    uint8_t raw;
    struct {
        uint8_t bg_enable : 1;
        uint8_t obj_enable : 1;
        uint8_t obj_size : 1;
        uint8_t bg_tilemap : 1;
        uint8_t bg_tiledata : 1;
        uint8_t win_enable : 1;
        uint8_t win_tilemap : 1;
        uint8_t lcd_enable : 1;
    } _packed_;
} LCDCRegister;

typedef union StatRegister {
    uint8_t raw;
    struct {
        uint8_t mode : 2;
        uint8_t lyc_ly_eq : 1;
        uint8_t hblank_int : 1;
        uint8_t vblank_int : 1;
        uint8_t oam_int : 1;
        uint8_t lyc_int : 1;
        uint8_t _unused_ : 1;
    } _packed_;
} StatRegister;

typedef struct PPU {
    RGB frame[144][160];
    uint8_t vram[0x2000];
    uint8_t oam[0xA0];

    LCDCRegister LCDC;
    StatRegister STAT;
    uint8_t SCY;
    uint8_t SCX;
    uint8_t LY;
    uint8_t LYC;
    uint8_t DMA;
    uint8_t BGP;
    uint8_t OBP0;
    uint8_t OBP1;
    uint8_t WY;
    uint8_t WX;

    bool frame_complete;
    bool vblank_interrupt;
    bool stat_interrupt;
    int line_ticks;
} PPU;

void ppu_reset(PPU *ppu);

void ppu_write(PPU *ppu, uint16_t addr, uint8_t val);

uint8_t ppu_read(PPU *ppu, uint16_t addr);

void ppu_step(PPU *ppu);
