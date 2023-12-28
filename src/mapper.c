#include <stdlib.h>
#include <stdint.h>

#include "common.h"
#include "mapper.h"
#include "mbc0.h"
#include "rom.h"

extern const gb_mapper_vt gb_mbc0_vtable;

static const
gb_mapper_vt *gb_mappers[0xFF] = {
    [0x00] = &gb_mbc0_vtable,
};

int
gb_mapper_init(gb_mapper_t *mapper, gb_rom_t *rom)
{
    uint8_t mapper_id = rom->header->cartridge_type;
    const gb_mapper_vt *vt = gb_mappers[mapper_id];
    if (vt == NULL) {
        GB_TRACE("unsupported mapper: %02X\n", mapper_id);
        return GB_ERR;
    }

    mapper->vt = vt;
    assert(vt->init != NULL);
    return vt->init(mapper, rom);
}
