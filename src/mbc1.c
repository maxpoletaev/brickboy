#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "common.h"
#include "mapper.h"
#include "mbc1.h"
#include "rom.h"

static IMapper mbc1_mapper = {
    .write = mbc1_write,
    .read = mbc1_read,
    .free = mbc1_free,
    .reset = mbc1_reset,
};

IMapper *
mbc1_new(ROM *rom)
{
    MBC1 *impl = xalloc(sizeof(MBC1));
    impl->imapper = mbc1_mapper;
    impl->rom = rom;

    impl->ram = xalloc(rom->ram_size);
    impl->ram_size = rom->ram_size;

    mbc1_reset(&impl->imapper);
    return &impl->imapper;
}

void
mbc1_free(IMapper *mapper)
{
    MBC1 *impl = CONTAINER_OF(mapper, MBC1, imapper);
    xfree(impl->ram);
    xfree(impl);
}

void mbc1_reset(IMapper *mapper)
{
    MBC1 *impl = CONTAINER_OF(mapper, MBC1, imapper);
    memset(impl->ram, 0, impl->ram_size);

    impl->rom_bank = 1;
    impl->ram_bank = 0;
    impl->ram_enabled = false;
}

uint8_t
mbc1_read(IMapper *mapper, uint16_t addr)
{
    MBC1 *impl = CONTAINER_OF(mapper, MBC1, imapper);

    uint32_t rom_addr;
    uint32_t ram_addr;

    switch (addr) {
    case 0x0000 ... 0x3FFF: // ROM bank 0 (fixed)
        return impl->rom->data[addr];
    case 0x4000 ... 0x7FFF: // ROM bank 1-31
        rom_addr = ROM_BANK_SIZE * impl->rom_bank + addr-0x4000;
        BOUNDS_CHECK(impl->rom->size, rom_addr);
        return impl->rom->data[rom_addr];
    case 0xA000 ... 0xBFFF: // RAM bank
        if (impl->ram_enabled) {
            ram_addr = RAM_BANK_SIZE * impl->ram_bank + addr-0xA000;
            BOUNDS_CHECK(impl->ram_size, ram_addr);
            return impl->ram[ram_addr];
        }
        return 0xFF;
    default:
        PANIC("unknown read address: 0x%04X", addr);
    }
}

void
mbc1_write(IMapper *mapper, uint16_t addr, uint8_t data)
{
    MBC1 *impl = CONTAINER_OF(mapper, MBC1, imapper);
    uint32_t ram_addr;

    switch (addr) {
    case 0x0000 ... 0x1FFF: // RAM enable
        impl->ram_enabled = (data & 0x0F) == 0x0A;
        break;
    case 0x2000 ... 0x3FFF: // ROM bank
        impl->rom_bank = data & 0x1F;
        if (impl->rom_bank == 0) {
            impl->rom_bank = 1;
        }
        break;
    case 0x4000 ... 0x5FFF: // RAM bank
        impl->ram_bank = data;
        break;
    case 0xA000 ... 0xBFFF: // RAM data
        if (impl->ram_enabled) {
            ram_addr = RAM_BANK_SIZE * impl->ram_bank + addr-0xA000;
            BOUNDS_CHECK(impl->ram_size, ram_addr);
            impl->ram[ram_addr] = data;
        }
        break;
    default:
        PANIC("unknown write address: 0x%04X", addr);
    }
}
