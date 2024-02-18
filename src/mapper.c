#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "common.h"
#include "mapper.h"
#include "rom.h"
#include "mbc0.h"

int
mapper_init(Mapper *mapper, ROM *rom)
{
    switch (rom->header->type) {
    case 0x00:
    case 0x01: // TODO: implement MBC1
        mapper->impl = must_alloc(sizeof(MBC0));
        mapper->vt = &mbc0_vtable;
        mapper->name = "ROM";
        mapper->id = 0x00;
        break;
    default:
        TRACE("unknown cartridge type: %d", rom->header->type);
        return RET_ERR;
    }

    assert(mapper->vt->init != NULL);
    return mapper->vt->init(mapper, rom);
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
    assert(mapper->vt != NULL);
    assert(mapper->vt->free != NULL);

    mapper->vt->free(mapper);
    free(mapper->impl);
    mapper->impl = NULL;
}
