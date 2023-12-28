#ifndef BRICKBOY_MAPPER_H
#define BRICKBOY_MAPPER_H

#include <stdint.h>
#include <assert.h>
#include "rom.h"

#define GB_MAPPER_IMPL(typename) \
    assert(mapper != NULL); \
    assert(mapper->impl != NULL); \
    typename *impl = (typename *) mapper->impl;

typedef struct gb_mapper_t gb_mapper_t;

typedef struct {
    int (*init)(gb_mapper_t *mapper, gb_rom_t *rom);
    void (*write)(gb_mapper_t *mapper, uint16_t addr, uint8_t data);
    uint8_t (*read)(gb_mapper_t *mapper, uint16_t addr);
    void (*reset)(gb_mapper_t *mapper);
    void (*free)(gb_mapper_t *mapper);
} gb_mapper_vt;

struct gb_mapper_t {
    void *impl;
    const gb_mapper_vt *vt;
};

int gb_mapper_init(gb_mapper_t *mapper, gb_rom_t *rom);

static inline void
gb_mapper_write(gb_mapper_t *mapper, uint16_t addr, uint8_t data)
{
    const gb_mapper_vt *vt = mapper->vt;
    assert(vt->write != NULL);
    vt->write(mapper, addr, data);
}

static inline uint8_t
gb_mapper_read(gb_mapper_t *mapper, uint16_t addr)
{
    const gb_mapper_vt *vt = mapper->vt;
    assert(vt->read != NULL);
    return vt->read(mapper, addr);
}

static inline void
gb_mapper_reset(gb_mapper_t *mapper)
{
    const gb_mapper_vt *vt = mapper->vt;
    assert(vt->reset != NULL);
    vt->reset(mapper);
}

static inline void
gb_mapper_free(gb_mapper_t *mapper)
{
    if (mapper->vt == NULL)
        return;

    const gb_mapper_vt *vt = mapper->vt;
    assert(vt->free != NULL);
    vt->free(mapper);

    // The implementation should have set these to NULL.
    assert(mapper->impl == NULL);
    assert(mapper->vt == NULL);
}

#endif //BRICKBOY_MAPPER_H
