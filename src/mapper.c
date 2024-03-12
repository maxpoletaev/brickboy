#include <stdint.h>

#include "mapper.h"


inline void
mapper_write(IMapper *mapper, uint16_t addr, uint8_t data)
{
    mapper->write(mapper, addr, data);
}

inline uint8_t
mapper_read(IMapper *mapper, uint16_t addr)
{
    return mapper->read(mapper, addr);
}

inline void
mapper_reset(IMapper *mapper)
{
    mapper->reset(mapper);
}

inline void
mapper_free(IMapper **mapper)
{
    (*mapper)->free(*mapper);
    *mapper = NULL;
}
