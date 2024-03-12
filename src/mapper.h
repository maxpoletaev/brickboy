#pragma once

#include <complex.h>
#include <stdint.h>
#include <assert.h>

#include "rom.h"

struct IMapper;

typedef struct IMapper {
    uint8_t id;
    const char *name;

    void (*write)(struct IMapper *mapper, uint16_t addr, uint8_t data);
    uint8_t (*read)(struct IMapper *mapper, uint16_t addr);
    void (*reset)(struct IMapper *mapper);
    void (*free)(struct IMapper *mapper);
} IMapper;

void mapper_write(IMapper *mapper, uint16_t addr, uint8_t data);

uint8_t mapper_read(IMapper *mapper, uint16_t addr);

void mapper_reset(IMapper *mapper);

void mapper_free(IMapper **mapper);
