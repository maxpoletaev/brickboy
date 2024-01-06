#include <stdint.h>
#include <string.h>

#include "shared.h"
#include "mapper.h"
#include "bus.h"

void
gb_bus_reset(gb_bus_t *bus)
{
    gb_mapper_reset(bus->mapper);
    memset(bus->ram, 0x00, sizeof(bus->ram));
    memset(bus->vram, 0x00, sizeof(bus->vram));
    memset(bus->oam, 0x00, sizeof(bus->oam));
    memset(bus->hram, 0xFF, sizeof(bus->hram));
    memset(bus->io, 0, sizeof(bus->io));
    bus->serial_data = 0;
    bus->serial_ctrl = 0;
    bus->ie = 0;
}

#define gb_if_addr(start, end) \
    if (addr >= (start) && addr <= (end)) \

uint8_t
gb_bus_read(gb_bus_t *bus, uint16_t addr)
{

#if GB_BUS_LY_STUB
    if (addr == 0xFF44)
        return 0x90;
#endif

    gb_if_addr(0x0000, 0x7FFF) // ROM
        return gb_mapper_read(bus->mapper, addr);

    gb_if_addr(0x8000, 0x9FFF) // Video RAM
        return bus->vram[addr - 0x8000];

    gb_if_addr(0xA000, 0xBFFF) // External RAM
        return gb_mapper_read(bus->mapper, addr);

    gb_if_addr(0xC000, 0xFDFF) { // Internal RAM
        if (addr >= 0xE000)
            addr -= 0x2000;

        return bus->ram[addr - 0xC000];
    }

    gb_if_addr(0xFE00, 0xFE9F) // OAM
        return bus->oam[addr - 0xFE00];

    gb_if_addr(0xFF00, 0xFF7F) // I/O
        return 0;

    gb_if_addr(0xFF80, 0xFFFE) // HRAM
        return bus->hram[addr - 0xFF80];

    GB_TRACE("unhandled read from 0x%04X", addr);

    return 0;
}

static void
gb_serial_write(gb_bus_t *bus, uint16_t addr, uint8_t data)
{
    switch (addr) {
    case 0xFF01:
        bus->serial_data = data;
        break;
    case 0xFF02:
        bus->serial_ctrl = data;
        break;
    default:
        break;
    }
}

void
gb_bus_write(gb_bus_t *bus, uint16_t addr, uint8_t data)
{
    gb_if_addr(0x0000, 0x7FFF) {
        gb_mapper_write(bus->mapper, addr, data);
        return;
    }

    gb_if_addr(0xC000, 0xFDFF) {
        if (addr >= 0xE000)
            addr -= 0x2000;

        bus->ram[addr - 0xC000] = data;
        return;
    }

    gb_if_addr(0x8000, 0x9FFF) {
        bus->vram[addr - 0x8000] = data;
        return;
    }

    gb_if_addr(0xA000, 0xBFFF) {
        gb_mapper_write(bus->mapper, addr, data);
        return;
    }

    gb_if_addr(0xFE00, 0xFE9F) {
        bus->oam[addr - 0xFE00] = data;
        return;
    }

    gb_if_addr(0xFF00, 0xFF7F) {
        gb_serial_write(bus, addr, data);
        return;
    }

    gb_if_addr(0xFF80, 0xFFFE) {
        bus->hram[addr - 0xFF80] = data;
        return;
    }

    GB_TRACE("unhandled write to 0x%04X", addr);
}

uint16_t
gb_bus_read16(gb_bus_t *bus, uint16_t addr)
{
    uint16_t val = 0;
    val |= (uint16_t) gb_bus_read(bus, addr + 0) << 0;
    val |= (uint16_t) gb_bus_read(bus, addr + 1) << 8;
    return val;
}

void
gb_bus_write16(gb_bus_t *bus, uint16_t addr, uint16_t data)
{
    gb_bus_write(bus, addr + 0, (uint8_t) (data >> 0));
    gb_bus_write(bus, addr + 1, (uint8_t) (data >> 8));
}

void
gb_bus_step(gb_bus_t *bus)
{
    bus->cycles += 1;
}
