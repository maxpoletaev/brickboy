#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "common.h"
#include "binario.h"
#include "mapper.h"
#include "mbc1.h"
#include "rom.h"
#include "str.h"

static IMapper mbc1_mapper = {
    .write = mbc1_write,
    .read = mbc1_read,
    .free = mbc1_free,
    .reset = mbc1_reset,
    .load_state = mbc1_load,
    .save_state = mbc1_save,
};

IMapper *
mbc1_new(ROM *rom)
{
    MBC1 *impl = xalloc(sizeof(MBC1));
    impl->imapper = mbc1_mapper;
    impl->rom = rom;

    impl->ram = xalloc(rom->ram_size);
    impl->ram_size = rom->ram_size;

    if (impl->rom->header->type == ROM_TYPE_MBC1_RAM_BATT) {
        impl->has_battery = true;
    }

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
    impl->ram_enabled = false;
    impl->rom_bank = 1;
    impl->ram_bank = 0;
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

int
mbc1_save(IMapper *mapper, const char *filename)
{
    MBC1 *impl = CONTAINER_OF(mapper, MBC1, imapper);
    if (!impl->has_battery) {
        return RET_OK;
    }

    String tmpfile _cleanup_(str_free) = str_new();
    tmpfile = str_add(tmpfile, filename);
    tmpfile = str_add(tmpfile, ".tmp");

    FILE *f _autoclose_ = fopen(tmpfile.ptr, "wb");

    if (f == NULL) {
        TRACE("failed to open file: %s", filename);
        return RET_ERR;
    }

    fwrite_u16(impl->rom->header->global_checksum, f);
    fwrite(impl->ram, impl->ram_size, 1, f);

    if (ferror(f)) {
        TRACE("failed to write to file: %s", filename);
        return RET_ERR;
    }

    if (rename(tmpfile.ptr, filename) != 0) {
        TRACE("failed to rename file: %s", filename);
        return RET_ERR;
    }

    LOG("saved battery file: %s", filename);

    return RET_OK;
}

int
mbc1_load(IMapper *mapper, const char *filename)
{
    MBC1 *impl = CONTAINER_OF(mapper, MBC1, imapper);
    if (!impl->has_battery) {
        return RET_OK;
    }

    FILE *f _autoclose_ = fopen(filename, "rb");

    if (f == NULL) {
        if (errno == ENOENT) {
            return RET_OK;
        }

        TRACE("failed to open file: %s (%s)", filename, strerror(errno));
        return RET_ERR;
    }

    uint16_t checksum;
    fread_u16(&checksum, f);
    fread(impl->ram, impl->ram_size, 1, f);

    if (ferror(f)) {
        TRACE("failed to read from file: %s", filename);
        return RET_ERR;
    }

    if (checksum != impl->rom->header->global_checksum) {
        TRACE("save file checksum mismatch");
        return RET_ERR;
    }

    LOG("loaded battery file: %s", filename);

    return 0;
}
