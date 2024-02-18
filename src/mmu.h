#ifndef OHMYBOY_MMU_H
#define OHMYBOY_MMU_H

#include <stdint.h>

#include "rom.h"
#include "timer.h"
#include "mapper.h"
#include "serial.h"

typedef enum {
    INT_VBLANK = (1 << 0),
    INT_LCD_STAT = (1 << 1),
    INT_TIMER = (1 << 2),
    INT_SERIAL = (1 << 3),
    INT_JOYPAD = (1 << 4),
} Interrupt;

typedef struct MMU {
    Mapper *mapper;       // Cartridge ROM (0x0000 - 0x7FFF) + ERAM (0xA000 - 0xBFFF)
    uint8_t vram[0x2000]; // 8KB VRAM (0x8000 - 0x9FFF)
    uint8_t ram[0x2000];  // 8KB WRAM (0xC000 - 0xDFFF) + Mirror (0xE000 - 0xFDFF)
    uint8_t oam[0xA0];    // 160B OAM (0xFE00 - 0xFE9F)
    uint8_t io[0x80];     // 128B I/O (0xFF00 - 0xFF7F)
    uint8_t hram[0x7F];   // 127B HRAM (0xFF80 - 0xFFFE)
    Serial serial;        // Serial (0xFF01-0xFF02)
    Timer timer;          // Timer (0xFF04 - 0xFF07)

    uint8_t intf; // Interrupt Flags (0xFF0F)
    uint8_t inte;  // Interrupt Enable (0xFFFF)
} MMU;

void mmu_reset(MMU *mmu);

uint8_t mmu_read(MMU *mmu, uint16_t addr);

void mmu_write(MMU *mmu, uint16_t addr, uint8_t data);

uint16_t mmu_read16(MMU *mmu, uint16_t addr);

void mmu_write16(MMU *mmu, uint16_t addr, uint16_t data);

#endif //OHMYBOY_MMU_H
