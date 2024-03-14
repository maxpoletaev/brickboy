#include "mbc0.h"

#include <stdint.h>

#include "common.h"
#include "mapper.h"
#include "rom.h"

static IMapper mbc0_mapper = {
    .id = 0x00,
    .name = "MBC0",
    .write = mbc0_write,
    .read = mbc0_read,
    .reset = mbc0_reset,
    .free = mbc0_free,
};

IMapper *
mbc0_create(ROM *rom)
{
    MBC0 *impl = xalloc(sizeof(MBC0));
    impl->imapper = mbc0_mapper;
    impl->rom = rom;

    return &impl->imapper;
}

void
mbc0_free(IMapper *mapper)
{
    MBC0 *impl = CONTAINER_OF(mapper, MBC0, imapper);
    xfree(impl);
}

void
mbc0_reset(IMapper *mapper)
{
    UNUSED(mapper);
}

uint8_t
mbc0_read(IMapper *mapper, uint16_t addr)
{
    MBC0 *impl = CONTAINER_OF(mapper, MBC0, imapper);

#ifndef NDEBUG
    if (addr >= impl->rom->size) {
        PANIC("out of bounds read at 0x%04X", addr);
    }
#endif

    return impl->rom->data[addr];
}

void
mbc0_write(IMapper *mapper, uint16_t addr, uint8_t data)
{
    UNUSED(mapper);
    UNUSED(data);
    TRACE("write to read-only memory at 0x%04X", addr);
}
