#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "mapper.h"
#include "mbc0.h"

const
gb_mapper_vt gb_mbc0_vtable = {
    .init = gb_mbc0_init,
    .write = gb_mbc0_write,
    .read = gb_mbc0_read,
    .reset = gb_mbc0_reset,
    .free = gb_mbc0_free,
};

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
    mapper->vt = NULL;
}

void
gb_mbc0_reset(gb_mapper_t *mapper)
{
    // does nothing, as mbc0 has no state
}

uint8_t
gb_mbc0_read(gb_mapper_t *mapper, uint16_t addr)
{
    GB_MAPPER_IMPL(gb_mbc0_t);

    if (addr >= impl->rom->size) {
        GB_TRACE("invalid address 0x%04X", addr);
        return 0;
    }

    return impl->rom->data[addr];
}

void
gb_mbc0_write(gb_mapper_t *mapper, uint16_t addr, uint8_t data)
{
    GB_TRACE("unhandled write 0x%04X\n", addr);
}
