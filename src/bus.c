#include <stdint.h>
#include <stdio.h>

#include "common.h"
#include "mapper.h"
#include "bus.h"

void
gb_bus_reset(gb_bus_t *bus)
{
    gb_mapper_reset(bus->mapper);
}

uint8_t
gb_bus_read(gb_bus_t *bus, uint16_t addr)
{
    if (addr >= 0x0000 && addr <= 0x7FFF) {
        return gb_mapper_read(bus->mapper, addr);
    } else if (addr >= 0xC000 && addr <= 0xDFFF) {
        return bus->ram[addr - 0xC000];
    } else if (addr >= 0xFE00 && addr <= 0xFE9F) {
        return bus->oam[addr - 0xFE00];
    } else {
        GB_TRACE("unhandled read from 0x%04X", addr);
        return 0;
    }
}

void
gb_bus_write(gb_bus_t *bus, uint16_t addr, uint8_t data)
{
    if (addr >= 0x0000 && addr <= 0x7FFF) {
        gb_mapper_write(bus->mapper, addr, data);
    } else if (addr >= 0xC000 && addr <= 0xDFFF) {
        bus->ram[addr - 0xC000] = data;
    } else if (addr >= 0xFE00 && addr <= 0xFE9F) {
        bus->oam[addr - 0xFE00] = data;
    } else {
        GB_TRACE("unhandled write to 0x%04X", addr);
    }
}
