#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "shared.h"
#include "mapper.h"
#include "rom.h"
#include "mbc0.h"

const gb_mapper_vt gb_mbc0_vtable = {
    .init = gb_mbc0_init,
    .write = gb_mbc0_write,
    .read = gb_mbc0_read,
    .reset = gb_mbc0_reset,
    .free = gb_mbc0_free,
};

static inline gb_mbc0_t *
gb_impl(gb_mapper_t *mapper)
{
    return (gb_mbc0_t *) mapper->impl;
}

int
gb_mbc0_init(gb_mapper_t *mapper, gb_rom_t *rom)
{
    gb_mbc0_t *impl = calloc(1, sizeof(gb_mbc0_t));
    if (impl == NULL) {
        GB_TRACE("failed to allocate memory");
        return GB_ERR;
    }

    impl->rom = rom;
    mapper->impl = impl;

    return GB_OK;
}

void
gb_mbc0_free(gb_mapper_t *mapper)
{
    free(mapper->impl);
    mapper->impl = NULL;
}

void
gb_mbc0_reset(gb_mapper_t *mapper)
{
    GB_UNUSED(mapper);
}

uint8_t
gb_mbc0_read(gb_mapper_t *mapper, uint16_t addr)
{
    gb_mbc0_t *impl = gb_impl(mapper);

    if (addr >= impl->rom->size)
        GB_PANIC("out of bounds read at 0x%04X", addr);

    return impl->rom->data[addr];
}

void
gb_mbc0_write(gb_mapper_t *mapper, uint16_t addr, uint8_t data)
{
    gb_mbc0_t *impl = gb_impl(mapper);

    if (addr >= impl->rom->size)
        GB_PANIC("out of bounds write at 0x%04X", addr);

    impl->rom->data[addr] = data;
}
