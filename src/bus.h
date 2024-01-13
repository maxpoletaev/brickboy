#ifndef OHMYBOY_BUS_H
#define OHMYBOY_BUS_H

#include <stdint.h>
#include "mapper.h"

#include "rom.h"
#include "timer.h"

#define BUS_TESTROM_MODE 1

typedef struct {
    uint8_t vblank : 1;
    uint8_t lcd_stat : 1;
    uint8_t timer : 1;
    uint8_t serial : 1;
    uint8_t joypad : 1;
    uint8_t unused : 3;
} IntFlags;

typedef struct Bus {
    Mapper *mapper;  // Cartridge ROM (0x0000 - 0x7FFF) + ERAM (0xA000 - 0xBFFF)
    uint8_t vram[0x2000]; // 8KB VRAM (0x8000 - 0x9FFF)
    uint8_t ram[0x2000];  // 8KB WRAM (0xC000 - 0xDFFF) + Mirror (0xE000 - 0xFDFF)
    uint8_t oam[0xA0];    // 160B OAM (0xFE00 - 0xFE9F)
    uint8_t io[0x80];     // 128B I/O (0xFF00 - 0xFF7F)
    uint8_t hram[0x7F];   // 127B HRAM (0xFF80 - 0xFFFE)

    uint8_t sb;  // Serial Data (0xFF01)
    uint8_t sc;  // Serial Control (0xFF02)
    Timer timer; // Timer (0xFF04 - 0xFF07)

    union {
        uint8_t ie;
        IntFlags ie_bits;
    }; // Interrupt Enable (0xFFFF)

    union {
        uint8_t if_;
        IntFlags if_bits;
    }; // Interrupt Flags (0xFF0F)
} Bus;

void bus_reset(Bus *bus);

uint8_t bus_read(Bus *bus, uint16_t addr);

void bus_write(Bus *bus, uint16_t addr, uint8_t data);

uint16_t bus_read16(Bus *bus, uint16_t addr);

void bus_write16(Bus *bus, uint16_t addr, uint16_t data);

#endif //OHMYBOY_BUS_H
