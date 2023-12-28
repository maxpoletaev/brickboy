#ifndef OHMYBOY_BUS_H
#define OHMYBOY_BUS_H

#include <stdint.h>
#include "mapper.h"
#include "rom.h"

typedef struct {
    gb_mapper_t *mapper; // Cartridge ROM (0x0000 - 0x7FFF)
    uint8_t ram[0x2000]; // 8KB RAM (0xC000 - 0xDFFF)
    uint8_t oam[0xA0]; // 160B OAM (0xFE00 - 0xFE9F)
    uint8_t io[0x80]; // 128B I/O (0xFF00 - 0xFF7F)
    uint8_t hram[0x7F]; // 127B HRAM (0xFF80 - 0xFFFE)
    uint8_t ie; // Interrupt Enable (0xFFFF)
} gb_bus_t;

void gb_bus_reset(gb_bus_t *bus);

uint8_t gb_bus_read(gb_bus_t *bus, uint16_t addr);

void gb_bus_write(gb_bus_t *bus, uint16_t addr, uint8_t data);

#endif //OHMYBOY_BUS_H
