#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#include "mapper.h"


inline void
mapper_write(IMapper *mapper, uint16_t addr, uint8_t data)
{
    assert(mapper->write != NULL);
    mapper->write(mapper, addr, data);
}

inline uint8_t
mapper_read(IMapper *mapper, uint16_t addr)
{
    assert(mapper->read != NULL);
    return mapper->read(mapper, addr);
}

inline void
mapper_reset(IMapper *mapper)
{
    assert(mapper->reset != NULL);
    mapper->reset(mapper);
}

int
mapper_save_state(IMapper *mapper, const char *filename)
{
    assert(mapper->save_state != NULL);
    return mapper->save_state(mapper, filename);
}

int
mapper_load_state(IMapper *mapper, const char *filename)
{
    assert(mapper->load_state != NULL);
    return mapper->load_state(mapper, filename);
}

inline void
mapper_free(IMapper **mapper)
{
    if (*mapper != NULL) {
        assert((*mapper)->free != NULL);
        (*mapper)->free(*mapper);
        *mapper = NULL;
    }
}
