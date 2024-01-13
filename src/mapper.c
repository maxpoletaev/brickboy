#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "shared.h"
#include "mapper.h"
#include "rom.h"
#include "mbc0.h"

static const MapperVT *mapper_vtables[256] = {
    [0x00] = &mbc0_vtable,
    [0x01] = &mbc0_vtable,
};

static const char *mapper_names[256] = {
    [0x00] = "ROM",
    [0x01] = "ROM+MBC1",
};

int
mapper_init(Mapper *mapper, ROM *rom)
{
    uint8_t mapper_id = rom->header->cartridge_type;
    const MapperVT *vt = mapper_vtables[mapper_id];
    const char *name = mapper_names[mapper_id];

    if (vt == NULL || name == NULL) {
        TRACE("unsupported mapper: %02X", mapper_id);
        return RET_ERR;
    }

    mapper->impl = NULL;
    assert(vt->init != NULL);
    int ret = vt->init(mapper, rom);

    if (ret == RET_OK) {
        mapper->vt = vt;
        mapper->name = name;
        assert(mapper->impl != NULL); // set by init
    }

    return ret;
}

inline void
mapper_write(Mapper *mapper, uint16_t addr, uint8_t data)
{
    assert(mapper->vt->write != NULL);
    mapper->vt->write(mapper, addr, data);
}

inline uint8_t
mapper_read(Mapper *mapper, uint16_t addr)
{
    assert(mapper->vt->read != NULL);
    return mapper->vt->read(mapper, addr);
}

inline void
mapper_reset(Mapper *mapper)
{
    assert(mapper->vt->reset != NULL);
    mapper->vt->reset(mapper);
}

inline void
mapper_free(Mapper *mapper)
{
    if (mapper->vt == NULL)
        return;

    assert(mapper->vt->free != NULL);
    mapper->vt->free(mapper);

    assert(mapper->impl == NULL); // nulled by vt->free
    mapper->name = NULL;
    mapper->vt = NULL;
}
