#pragma once

#include <complex.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include "rom.h"

struct IMapper;

typedef struct IMapper {
    void (*write)(struct IMapper *mapper, uint16_t addr, uint8_t data);
    uint8_t (*read)(struct IMapper *mapper, uint16_t addr);
    void (*reset)(struct IMapper *mapper);
    void (*free)(struct IMapper *mapper);

    // For battery-backed cartridges:
    int (*save_state)(struct IMapper *mapper, const char *filename);
    int (*load_state)(struct IMapper *mapper, const char *filename);
} IMapper;

void mapper_write(IMapper *mapper, uint16_t addr, uint8_t data);

uint8_t mapper_read(IMapper *mapper, uint16_t addr);

void mapper_reset(IMapper *mapper);

void mapper_free(IMapper **mapper);

int mapper_save_state(IMapper *mapper, const char *filename);

int mapper_load_state(IMapper *mapper, const char *filename);
