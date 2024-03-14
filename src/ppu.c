#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "ppu.h"
#include "common.h"

const RGB ppu_colors[4] = {
    {255, 255, 255},
    {192, 192, 192},
    {96, 96, 96},
    {0, 0, 0},
};

enum PPUMode {
    PPU_MODE_HBLANK = 0,
    PPU_MODE_VBLANK = 1,
    PPU_MODE_OAM_SCAN = 2,
    PPU_MODE_PIXEL_DRAW = 3
};

union LCDCRegister {
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
};

union StatRegister {
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
};

struct PPU {
    RGB frame[144][160];
    uint8_t vram[0x2000];
    uint8_t oam[0xA0];

    union LCDCRegister LCDC;
    union StatRegister STAT;
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

    bool vblank_interrupt;
    bool stat_interrupt;
    int line_ticks;
};

PPU *
ppu_new(void)
{
    PPU *ppu = xalloc(sizeof(PPU));
    ppu_reset(ppu);
    return ppu;
}

void
ppu_free(PPU **ppu)
{
    xfree(*ppu);
}

void
ppu_reset(PPU *ppu)
{
    memset(ppu->frame, 0x00, sizeof(ppu->frame));
    memset(ppu->vram, 0x00, sizeof(ppu->vram));
    memset(ppu->oam, 0x00, sizeof(ppu->oam));

    ppu->LCDC.raw = 0x91;
    ppu->STAT.raw = 0;
    ppu->SCX = 0;
    ppu->SCY = 0;
    ppu->LY = 0;
    ppu->LYC = 0;
    ppu->DMA = 0;
    ppu->WX = 0;
    ppu->WY = 0;
}

uint8_t
ppu_read(PPU *ppu, uint16_t addr)
{
    switch (addr) {
    case 0x8000 ... 0x9FFF: // VRAM
        return ppu->vram[addr - 0x8000];
    case 0xFE00 ... 0xFE9F: // OAM
        return ppu->oam[addr - 0xFE00];
    case 0xFF40: // LCDC
        return ppu->LCDC.raw;
    case 0xFF41: // STAT
        return ppu->STAT.raw;
    case 0xFF42: // SCY
        return ppu->SCY;
    case 0xFF43: // SCX
        return ppu->SCX;
    case 0xFF44: // LY
        return ppu->LY;
    case 0xFF45: // LYC
        return ppu->LYC;
    case 0xFF46: // DMA
        return ppu->DMA;
    case 0xFF47: // BGP
        return ppu->BGP;
    case 0xFF48: // OBP0
        return ppu->OBP0;
    case 0xFF49: // OBP1
        return ppu->OBP1;
    case 0xFF4A: // WY
        return ppu->WY;
    case 0xFF4B: // WX
        return ppu->WX;
    default:
        TRACE("unhandled read from 0x%04X", addr);
        return 0;
    }
}

static inline uint8_t
ppu_read_vram(PPU *ppu, uint16_t addr)
{
#ifndef NDEBUG
    if (addr < 0x8000 || addr > 0x9FFF) {
        PANIC("invalid VRAM read at 0x%04X", addr);
    }
#endif

    return ppu->vram[addr - 0x8000];
}

void
ppu_write(PPU *ppu, uint16_t addr, uint8_t data)
{
    switch (addr) {
    case 0x8000 ... 0x9FFF: // VRAM
        ppu->vram[addr - 0x8000] = data;
        break;
    case 0xFE00 ... 0xFE9F: // OAM
        ppu->oam[addr - 0xFE00] = data;
        break;
    case 0xFF40: // LCDC
        ppu->LCDC.raw = data;
        break;
    case 0xFF41: // STAT
        ppu->STAT.raw = (ppu->STAT.raw & 0x7) | (data & ~0x07); // 0-2 are read-only
        break;
    case 0xFF42: // SCY
        ppu->SCY = data;
        break;
    case 0xFF43: // SCX
        ppu->SCX = data;
        break;
    case 0xFF45: // LYC
        ppu->LYC = data;
        break;
    case 0xFF46: // DMA
        ppu->DMA = data;
        break;
    case 0xFF47: // BGP
        ppu->BGP = data;
        break;
    case 0xFF48: // OBP0
        ppu->OBP0 = data;
        break;
    case 0xFF49: // OBP1
        ppu->OBP1 = data;
        break;
    case 0xFF4A: // WY
        ppu->WY = data;
        break;
    case 0xFF4B: // WX
        ppu->WX = data;
        break;
    default:
        TRACE("unhandled write to 0x%04X", addr);
    }
}

void
ppu_clear_frame(PPU *ppu, RGB color)
{
    for (int y = 0; y < 144; y++) {
        for (int x = 0; x < 160; x++) {
            ppu->frame[y][x] = color;
        }
    }
}

static inline uint16_t
ppu_tile_addr(PPU *ppu, uint8_t tile_id)
{
    if (ppu->LCDC.bg_tiledata) {
        return 0x8000 + tile_id * 16;
    }

    return 0x9000 + (int8_t) tile_id * 16;
}

static void
ppu_fetch_tile_line(PPU *ppu, int tile_y, int tile_x, int pixel_y, uint8_t pixels[8])
{
    uint16_t tile_map = ppu->LCDC.bg_tilemap ? 0x9C00 : 0x9800;
    uint16_t tile_map_addr = tile_map + tile_y*32 + tile_x;

    uint8_t tile_id = ppu_read_vram(ppu, tile_map_addr);
    uint16_t tile_addr = ppu_tile_addr(ppu, tile_id);

    uint8_t b0 = ppu_read_vram(ppu, tile_addr + pixel_y*2 + 0);
    uint8_t b1 = ppu_read_vram(ppu, tile_addr + pixel_y*2 + 1);

    for (int x = 0; x < 8; x++) {
        uint8_t v0 = ((b0 >> x) & 0x1) << 1;
        uint8_t v1 = ((b1 >> x) & 0x1) << 0;
        pixels[7-x] = v0 | v1;
    }
}

static void
ppu_render_scanline(PPU *ppu)
{
    if (ppu->LCDC.bg_enable) {
        uint8_t screen_y = ppu->LY;

        int tile_y = ((ppu->LY + ppu->SCY) / 8) % 32;
        int pixel_y = (ppu->LY + ppu->SCY) % 8;

        uint8_t tile_line[8];
        int last_tile_x = -1;

        for (int screen_x = 0; screen_x < 160; screen_x++) {
            int tile_x = ((ppu->SCX + screen_x) / 8) % 32;
            int pixel_x = (ppu->SCX + screen_x) % 8;

            if (tile_x != last_tile_x) {
                ppu_fetch_tile_line(ppu, tile_y, tile_x, pixel_y, tile_line);
                last_tile_x = tile_x;
            }

            uint8_t color_id = tile_line[pixel_x];
            uint8_t color = (ppu->BGP >> (color_id * 2)) & 0x3;

            ppu->frame[screen_y][screen_x] = ppu_colors[color];
        }
    }
}

static inline void
ppu_set_mode(PPU *ppu, enum PPUMode mode)
{
    switch (mode) {
    case PPU_MODE_OAM_SCAN:
        if (ppu->STAT.oam_int) {
            ppu->stat_interrupt = true;
        }
        break;
    case PPU_MODE_VBLANK:
        if (ppu->STAT.vblank_int) {
            ppu->stat_interrupt = true;
        }
        break;
    case PPU_MODE_HBLANK:
        if (ppu->STAT.hblank_int) {
            ppu->stat_interrupt = true;
        }
        break;
    default:
        break;
    }

    ppu->STAT.mode = mode;
}

static inline void
ppu_ly_increment(PPU *ppu)
{
    ppu->LY++;

    if (ppu->LY == ppu->LYC) {
        ppu->STAT.lyc_ly_eq = 1;
        if (ppu->STAT.lyc_int) {
            ppu->stat_interrupt = true;
        }
    } else {
        ppu->STAT.lyc_ly_eq = 0;
    }
}

static inline void
ppu_step_oam_scan(PPU *ppu)
{
    if (ppu->line_ticks == 80) {
        ppu_set_mode(ppu, PPU_MODE_PIXEL_DRAW);
    }
}

static inline void
ppu_step_pixel_draw(PPU *ppu)
{
    if (ppu->line_ticks == 252) {
        ppu_set_mode(ppu, PPU_MODE_HBLANK);
        ppu_render_scanline(ppu);
    }
}

static inline void
ppu_step_hblank(PPU *ppu)
{
    if (ppu->line_ticks == 456) {
        ppu_ly_increment(ppu);
        ppu->line_ticks = 0;

        if (ppu->LY == 144) {
            ppu->vblank_interrupt = true;
            ppu_set_mode(ppu, PPU_MODE_VBLANK);
        } else {
            ppu_set_mode(ppu, PPU_MODE_OAM_SCAN);
        }
    }
}

static inline void
ppu_step_vblank(PPU *ppu)
{
    if (ppu->line_ticks == 456) {
        ppu_ly_increment(ppu);
        ppu->line_ticks = 0;

        if (ppu->LY == 153) {
            ppu_set_mode(ppu, PPU_MODE_OAM_SCAN);
            ppu_clear_frame(ppu, ppu_colors[0]);
            ppu->LY = 0;
        }
    }
}

void
ppu_step(PPU *ppu)
{
    ppu->line_ticks++;

    switch (ppu->STAT.mode) {
    case PPU_MODE_OAM_SCAN:
        ppu_step_oam_scan(ppu);
        break;
    case PPU_MODE_PIXEL_DRAW:
        ppu_step_pixel_draw(ppu);
        break;
    case PPU_MODE_HBLANK:
        ppu_step_hblank(ppu);
        break;
    case PPU_MODE_VBLANK:
        ppu_step_vblank(ppu);
        break;
    default:
        PANIC("invalid PPU mode %d", ppu->STAT.mode);
    }
}

inline const RGB *
ppu_get_frame(PPU *ppu)
{
    return (RGB *) ppu->frame;
}

inline const uint8_t *
ppu_get_vram(PPU *ppu)
{
    return ppu->vram;
}

inline bool
ppu_stat_interrupt(PPU *ppu)
{
    if (ppu->stat_interrupt) {
        ppu->stat_interrupt = false;
        return true;
    }

    return false;

}

inline bool
ppu_vblank_interrupt(PPU *ppu)
{
    if (ppu->vblank_interrupt) {
        ppu->vblank_interrupt = false;
        return true;
    }

    return false;
}
