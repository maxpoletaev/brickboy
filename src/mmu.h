#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "rom.h"
#include "timer.h"
#include "mapper.h"
#include "serial.h"
#include "ppu.h"

#define MMU_FIXED_LY 0

typedef struct MMU {
    IMapper *mapper;      // Cartridge ROM (0x0000 - 0x7FFF) + ERAM (0xA000 - 0xBFFF)
    uint8_t ram[0x2000];  // 8KB WRAM (0xC000 - 0xDFFF) + Mirror (0xE000 - 0xFDFF)
    uint8_t hram[0x7F];   // 127B HRAM (0xFF80 - 0xFFFE)
    Serial *serial;       // Serial (0xFF01-0xFF02)
    Timer *timer;         // Timer (0xFF04 - 0xFF07)
    PPU *ppu;             // PPU (0xFF40 - 0xFF4B) + VRAM (0x8000 - 0x9FFF) + OAM (0xFE00 - 0xFE9F)
    uint8_t IF;           // Interrupt Flags (0xFF0F)
    uint8_t IE;           // Interrupt Enable (0xFFFF)
    uint8_t input;        // Input (0xFF00)

    bool bootrom_mapped;
    uint8_t dma_cycles;
    uint8_t dma_page;
} MMU;

MMU *mmu_new(IMapper *mapper, Serial *serial, Timer *timer, PPU *ppu);

void mmu_free(MMU **mmu);

void mmu_reset(MMU *mmu);

uint8_t mmu_read(MMU *mmu, uint16_t addr);

void mmu_write(MMU *mmu, uint16_t addr, uint8_t data);

uint16_t mmu_read16(MMU *mmu, uint16_t addr);

void mmu_write16(MMU *mmu, uint16_t addr, uint16_t data);

void mmu_dma_tick(MMU *mmu);
