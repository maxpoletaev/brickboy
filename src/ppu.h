#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common.h"

typedef struct PPU PPU;

PPU *ppu_new(void);

void ppu_free(PPU **ppu);

void ppu_reset(PPU *ppu);

void ppu_write(PPU *ppu, uint16_t addr, uint8_t data);

uint8_t ppu_read(PPU *ppu, uint16_t addr);

void ppu_step(PPU *ppu);

const uint8_t *ppu_get_frame(PPU *ppu);

const uint8_t *ppu_get_vram(PPU *ppu);

bool ppu_stat_interrupt(PPU *ppu);

bool ppu_vblank_interrupt(PPU *ppu);
