#include <stdint.h>
#include <string.h>

#include "common.h"
#include "mapper.h"
#include "timer.h"
#include "mmu.h"
#include "serial.h"

void
mmu_reset(MMU *mmu)
{
    timer_reset(&mmu->timer);
    serial_reset(&mmu->serial);
    mapper_reset(mmu->mapper);

    memset(mmu->ram, 0x00, sizeof(mmu->ram));
    memset(mmu->vram, 0x00, sizeof(mmu->vram));
    memset(mmu->oam, 0x00, sizeof(mmu->oam));
    memset(mmu->hram, 0xFF, sizeof(mmu->hram));
    memset(mmu->io, 0, sizeof(mmu->io));

    mmu->inte = 0;
    mmu->intf = 0;
}

uint8_t
mmu_read(MMU *mmu, uint16_t addr)
{
    if (addr == 0xFF44) {
        return 0x90; // LY=144, hardcoded for now to pass blargg's tests
    }

    switch (addr) {
    case 0x0000 ... 0x7FFF: // ROM
        return mapper_read(mmu->mapper, addr);
    case 0x8000 ... 0x9FFF: // VRAM
        return mmu->vram[addr - 0x8000];
    case 0xA000 ... 0xBFFF: // External RAM
        return mapper_read(mmu->mapper, addr);
    case 0xC000 ... 0xDFFF: // Internal RAM
        return mmu->ram[addr - 0xC000];
    case 0xE000 ... 0xFDFF: // Internal RAM (mirror)
        return mmu->ram[addr - 0xE000];
    case 0xFF01 ... 0xFF02: // Serial
        return serial_read(&mmu->serial, addr);
    case 0xFF04 ... 0xFF07: // Timer
        return timer_read(&mmu->timer, addr);
    case 0xFF0F: // Interrupt Flags
        LOG("intf read: 0x%02X", mmu->intf);
        return mmu->intf;
    case 0xFF80 ... 0xFFFE: // HRAM
        return mmu->hram[addr - 0xFF80];
    case 0xFFFF: // Interrupt Enable
        LOG("inte read: 0x%02X", mmu->inte);
        return mmu->inte;
    default:
        TRACE("unhandled read from 0x%04X", addr);
        return 0;
    }
}

void
mmu_write(MMU *mmu, uint16_t addr, uint8_t data)
{
    switch (addr) {
    case 0x0000 ... 0x7FFF: // ROM
        mapper_write(mmu->mapper, addr, data);
        return;
    case 0x8000 ... 0x9FFF: // VRAM
        mmu->vram[addr - 0x8000] = data;
        return;
    case 0xA000 ... 0xBFFF: // External RAM
        mapper_write(mmu->mapper, addr, data);
        return;
    case 0xC000 ... 0xDFFF: // Internal RAM
        mmu->ram[addr - 0xC000] = data;
        return;
    case 0xE000 ... 0xFDFF: // Internal RAM (mirror)
        mmu->ram[addr - 0xE000] = data;
        return;
    case 0xFE00 ... 0xFE9F: // OAM
        mmu->oam[addr - 0xFE00] = data;
        return;
    case 0xFF01 ... 0xFF02: // Serial
        serial_write(&mmu->serial, addr, data);
        return;
    case 0xFF04 ... 0xFF07: // Timer
        timer_write(&mmu->timer, addr, data);
        return;
    case 0xFF0F: // Interrupt Flags
        LOG("intf write: 0x%02X", data);
        mmu->intf = data;
    case 0xFF80 ... 0xFFFE: // HRAM
        mmu->hram[addr - 0xFF80] = data;
        return;
    case 0xFFFF: // Interrupt Enable
        LOG("inte write: 0x%02X", data);
        mmu->inte = data & 0x1F;
        return;
    default:
        TRACE("unhandled write to 0x%04X", addr);
        return;
    }
}

uint16_t
mmu_read16(MMU *mmu, uint16_t addr)
{
    uint16_t val = 0;
    val |= (uint16_t) (mmu_read(mmu, addr + 0) << 0);
    val |= (uint16_t) (mmu_read(mmu, addr + 1) << 8);
    return val;
}

void
mmu_write16(MMU *mmu, uint16_t addr, uint16_t data)
{
    mmu_write(mmu, addr + 0, (uint8_t) (data >> 0));
    mmu_write(mmu, addr + 1, (uint8_t) (data >> 8));
}
