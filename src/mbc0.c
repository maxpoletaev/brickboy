#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "shared.h"
#include "mapper.h"
#include "rom.h"
#include "mbc0.h"

const MapperVT mbc0_vtable = {
    .init = mbc0_init,
    .write = mbc0_write,
    .read = mbc0_read,
    .reset = mbc0_reset,
    .free = mbc0_free,
};

static inline MBC0 *
mbc0_impl(Mapper *mapper)
{
    return (MBC0 *) mapper->impl;
}

int
mbc0_init(Mapper *mapper, ROM *rom)
{
    MBC0 *impl = calloc(1, sizeof(MBC0));
    if (impl == NULL) {
        TRACE("failed to allocate memory");
        return RET_ERR;
    }

    impl->rom = rom;
    mapper->impl = impl;

    return RET_OK;
}

void
mbc0_free(Mapper *mapper)
{
    free(mapper->impl);
    mapper->impl = NULL;
}

void
mbc0_reset(Mapper *mapper)
{
    UNUSED(mapper);
}

uint8_t
mbc0_read(Mapper *mapper, uint16_t addr)
{
    MBC0 *impl = mbc0_impl(mapper);

    if (addr >= impl->rom->size)
        PANIC("out of bounds read at 0x%04X", addr);

    return impl->rom->data[addr];
}

void
mbc0_write(Mapper *mapper, uint16_t addr, uint8_t data)
{
    MBC0 *impl = mbc0_impl(mapper);

    if (addr >= impl->rom->size)
        PANIC("out of bounds write at 0x%04X", addr);

    impl->rom->data[addr] = data;
}
