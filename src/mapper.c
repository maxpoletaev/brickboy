#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "shared.h"
#include "mapper.h"
#include "rom.h"
#include "mbc0.h"

static const gb_mapper_vt *gb_mappers[256] = {
    [0x00] = &gb_mbc0_vtable,
    [0x01] = &gb_mbc0_vtable,
};

static const char *gb_mapper_names[256] = {
    [0x00] = "ROM ONLY",
    [0x01] = "ROM+MBC1",
};

int
gb_mapper_init(gb_mapper_t *mapper, gb_rom_t *rom)
{
    uint8_t mapper_id = rom->header->cartridge_type;
    const gb_mapper_vt *vt = gb_mappers[mapper_id];
    const char *name = gb_mapper_names[mapper_id];

    if (vt == NULL || name == NULL) {
        GB_TRACE("unsupported mapper: %02X\n", mapper_id);
        return GB_ERR;
    }

    mapper->impl = NULL;
    assert(vt->init != NULL);
    int ret = vt->init(mapper, rom);

    if (ret == GB_OK) {
        mapper->vt = vt;
        mapper->name = name;
        assert(mapper->impl != NULL); // set by init
    }

    return ret;
}

inline void
gb_mapper_write(gb_mapper_t *mapper, uint16_t addr, uint8_t data)
{
    assert(mapper->vt->write != NULL);
    mapper->vt->write(mapper, addr, data);
}

inline uint8_t
gb_mapper_read(gb_mapper_t *mapper, uint16_t addr)
{
    assert(mapper->vt->read != NULL);
    return mapper->vt->read(mapper, addr);
}

inline void
gb_mapper_reset(gb_mapper_t *mapper)
{
    assert(mapper->vt->reset != NULL);
    mapper->vt->reset(mapper);
}

inline void
gb_mapper_free(gb_mapper_t *mapper)
{
    if (mapper->vt == NULL)
        return;

    assert(mapper->vt->free != NULL);
    mapper->vt->free(mapper);
    mapper->name = NULL;
    mapper->vt = NULL;

    assert(mapper->impl == NULL); // nulled by free
}
