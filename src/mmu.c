#include <stdint.h>
#include <string.h>

#include "common.h"
#include "mapper.h"
#include "timer.h"
#include "mmu.h"
#include "serial.h"
#include "ppu.h"
#include "joypad.h"

MMU *
mmu_new(IMapper *mapper, Serial *serial, Timer *timer, PPU *ppu, Joypad *joypad)
{
    MMU *mmu = xalloc(sizeof(MMU));
    mmu->joypad = joypad;
    mmu->mapper = mapper;
    mmu->serial = serial;
    mmu->timer = timer;
    mmu->ppu = ppu;
    mmu_reset(mmu);
    return mmu;
}

void mmu_free(MMU **mmu)
{
    xfree(*mmu);
}

void
mmu_reset(MMU *mmu)
{
    ppu_reset(mmu->ppu);
    timer_reset(mmu->timer);
    serial_reset(mmu->serial);
    mapper_reset(mmu->mapper);
    joypad_reset(mmu->joypad);

    memset(mmu->ram, 0x00, sizeof(mmu->ram));
    memset(mmu->hram, 0xFF, sizeof(mmu->hram));

    mmu->IE = 0;
    mmu->IF = 0;
}

uint8_t
mmu_read(MMU *mmu, uint16_t addr)
{
#if MMU_FIXED_LY
    if (addr == 0xFF44) {
        return 0x90;
    }
#endif

    switch (addr) {
    case 0x0000 ... 0x7FFF: // ROM
    case 0xA000 ... 0xBFFF: // External RAM
        return mapper_read(mmu->mapper, addr);
    case 0xC000 ... 0xDFFF: // Internal RAM
        return mmu->ram[addr - 0xC000];
    case 0xE000 ... 0xFDFF: // Internal RAM (mirror)
        return mmu->ram[addr - 0xE000];
    case 0xFF01 ... 0xFF02: // Serial
        return serial_read(mmu->serial, addr);
    case 0xFF04 ... 0xFF07: // Timer
        return timer_read(mmu->timer, addr);
    case 0xFF0F: // Interrupt Flags
        return mmu->IF;
    case 0xFF10 ... 0xFF3F: // Sound
        return 0;
    case 0xFF40 ... 0xFF4B: // PPU registers
    case 0x8000 ... 0x9FFF: // VRAM
    case 0xFE00 ... 0xFE9F: // OAM
        return ppu_read(mmu->ppu, addr);
    case 0xFEA0 ... 0xFEFF: // Unusable
        return 0;
    case 0xFF80 ... 0xFFFE: // HRAM
        return mmu->hram[addr - 0xFF80];
    case 0xFF00: // Joypad
        return joypad_read(mmu->joypad);
    case 0xFFFF: // Interrupt Enable
        return mmu->IE;
    default:
        TRACE("unhandled read from 0x%04X", addr);
        return 0;
    }
}

void
mmu_write(MMU *mmu, uint16_t addr, uint8_t data)
{
    if (addr == 0xFF46) {
        mmu->dma_cycles = 160;
        mmu->dma_page = data;
    }

    switch (addr) {
    case 0x0000 ... 0x7FFF: // ROM
    case 0xA000 ... 0xBFFF: // External RAM
        mapper_write(mmu->mapper, addr, data);
        return;
    case 0xC000 ... 0xDFFF: // Internal RAM
        mmu->ram[addr - 0xC000] = data;
        return;
    case 0xE000 ... 0xFDFF: // Internal RAM (mirror)
        mmu->ram[addr - 0xE000] = data;
        return;
    case 0xFF01 ... 0xFF02: // Serial
        serial_write(mmu->serial, addr, data);
        return;
    case 0xFF04 ... 0xFF07: // Timer
        timer_write(mmu->timer, addr, data);
        return;
    case 0xFF0F: // Interrupt Flags
        mmu->IF = data;
        return;
    case 0xFF10 ... 0xFF3F: // Sound
        return;
    case 0xFF40 ... 0xFF4B: // PPU registers
    case 0x8000 ... 0x9FFF: // VRAM
    case 0xFE00 ... 0xFE9F: // OAM
        ppu_write(mmu->ppu, addr, data);
        return;
    case 0xFEA0 ... 0xFEFF: // Unusable
        return;
    case 0xFF80 ... 0xFFFE: // HRAM
        mmu->hram[addr - 0xFF80] = data;
        return;
    case 0xFF00: // Joypad
        joypad_write(mmu->joypad, data);
        return;
    case 0xFFFF: // Interrupt Enable
        mmu->IE = data & 0x1F;
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
    val |= (uint16_t) mmu_read(mmu, addr + 0) << 0;
    val |= (uint16_t) mmu_read(mmu, addr + 1) << 8;
    return val;
}

void
mmu_write16(MMU *mmu, uint16_t addr, uint16_t data)
{
    mmu_write(mmu, addr + 0, (uint8_t) (data >> 0));
    mmu_write(mmu, addr + 1, (uint8_t) (data >> 8));
}

static void
mmu_dma(MMU *mmu, uint16_t page)
{
    uint16_t addr = page << 8;
    for (int i = 0; i < 0xA0; i++) {
        mmu_write(mmu, 0xFE00 + i, mmu_read(mmu, addr + i));
    }
}

void
mmu_dma_tick(MMU *mmu)
{
    if (mmu->dma_cycles > 0) {
        mmu->dma_cycles--;

        if (mmu->dma_cycles == 0) {
            mmu_dma(mmu, mmu->dma_page);
        }
    }
}
