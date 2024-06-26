#include <stdint.h>

#include "common.h"
#include "mapper.h"
#include "mbc0.h"
#include "rom.h"

static IMapper mbc0_mapper = {
    .write = mbc0_write,
    .read = mbc0_read,
    .reset = mbc0_reset,
    .free = mbc0_free,
    .load_state = mbc0_load,
    .save_state = mbc0_save,
};

IMapper *
mbc0_new(ROM *rom)
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

    if (addr >= impl->rom->size) {
        PANIC("address out of range: 0x%04X", addr);
    }

    return impl->rom->data[addr];
}

void
mbc0_write(IMapper *mapper, uint16_t addr, uint8_t data)
{
    UNUSED(mapper);
    UNUSED(data);
    TRACE("write to read-only memory at 0x%04X", addr);
}

int
mbc0_load(IMapper *mapper, const char *filename)
{
    UNUSED(mapper);
    UNUSED(filename);
    return RET_OK;
}

int
mbc0_save(IMapper *mapper, const char *filename)
{
    UNUSED(mapper);
    UNUSED(filename);
    return RET_OK;
}
