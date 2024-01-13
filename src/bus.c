#include <stdint.h>
#include <string.h>

#include "shared.h"
#include "mapper.h"
#include "bus.h"

void
bus_reset(Bus *bus)
{
    mapper_reset(bus->mapper);
    memset(bus->ram, 0x00, sizeof(bus->ram));
    memset(bus->vram, 0x00, sizeof(bus->vram));
    memset(bus->oam, 0x00, sizeof(bus->oam));
    memset(bus->hram, 0x00, sizeof(bus->hram));
    memset(bus->io, 0, sizeof(bus->io));
    bus->sb = 0;
    bus->sc = 0;
    bus->ie = 0;

#if BUS_TESTROM_MODE
    // Should be 0xFF to as in wheremyfoodat’s logs
    memset(bus->hram, 0xFF, sizeof(bus->hram));
#endif
}

#define IF_ADDR(start, end) if (addr >= (start) && addr <= (end))

uint8_t
bus_read(Bus *bus, uint16_t addr)
{

#if BUS_TESTROM_MODE
    if (addr == 0xFF44)
        return 0x90;
#endif

    if (addr == 0xFFFF)
        return bus->ie;

    IF_ADDR(0x0000, 0x7FFF) // ROM
        return mapper_read(bus->mapper, addr);

    IF_ADDR(0x8000, 0x9FFF) // Video RAM
        return bus->vram[addr - 0x8000];

    IF_ADDR(0xA000, 0xBFFF) // External RAM
        return mapper_read(bus->mapper, addr);

    IF_ADDR(0xC000, 0xDFFF) // Internal RAM
        return bus->ram[addr - 0xC000];

    IF_ADDR(0xE000, 0xFDFF) // Internal RAM (mirror)
        return bus->ram[addr - 0xE000];

    IF_ADDR(0xFE00, 0xFE9F) // OAM
        return bus->oam[addr - 0xFE00];

    IF_ADDR(0xFF00, 0xFF7F) // I/O
        return 0;

    IF_ADDR(0xFF80, 0xFFFE) // HRAM
        return bus->hram[addr - 0xFF80];

    TRACE("unhandled read from 0x%04X", addr);

    return 0;
}

static void
write_serial(Bus *bus, uint16_t addr, uint8_t data)
{
    switch (addr) {
    case 0xFF01:
        bus->sb = data;
        break;
    case 0xFF02:
        bus->sc = data;
        break;
    default:
        break;
    }
}

void
bus_write(Bus *bus, uint16_t addr, uint8_t data)
{
    if (addr == 0xFFFF) {
        bus->ie = data & 0x1F;
        return;
    }

    IF_ADDR(0x0000, 0x7FFF) {
        mapper_write(bus->mapper, addr, data);
        return;
    }

    IF_ADDR(0xC000, 0xDFFF) {
        bus->ram[addr - 0xC000] = data;
        return;
    }

    IF_ADDR(0xE000, 0xFDFF) {
        bus->ram[addr - 0xE000] = data;
        return;
    }

    IF_ADDR(0x8000, 0x9FFF) {
        bus->vram[addr - 0x8000] = data;
        return;
    }

    IF_ADDR(0xA000, 0xBFFF) {
        mapper_write(bus->mapper, addr, data);
        return;
    }

    IF_ADDR(0xFE00, 0xFE9F) {
        bus->oam[addr - 0xFE00] = data;
        return;
    }

    IF_ADDR(0xFF00, 0xFF7F) {
        write_serial(bus, addr, data);
        return;
    }

    IF_ADDR(0xFF80, 0xFFFE) {
        bus->hram[addr - 0xFF80] = data;
        return;
    }

    TRACE("unhandled write to 0x%04X", addr);
}

uint16_t
bus_read16(Bus *bus, uint16_t addr)
{
    uint16_t val = 0;
    val |= (uint16_t) bus_read(bus, addr + 0) << 0;
    val |= (uint16_t) bus_read(bus, addr + 1) << 8;
    return val;
}

void
bus_write16(Bus *bus, uint16_t addr, uint16_t data)
{
    bus_write(bus, addr + 0, (uint8_t) (data >> 0));
    bus_write(bus, addr + 1, (uint8_t) (data >> 8));
}
