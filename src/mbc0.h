#pragma once

#include <stdint.h>

#include "rom.h"
#include "mapper.h"

typedef struct {
    IMapper mapper;
    ROM *rom;
} MBC0;

IMapper *mbc0_create(ROM *ROM);

void mbc0_write(IMapper *mapper, uint16_t addr, uint8_t data);

uint8_t mbc0_read(IMapper *mapper, uint16_t addr);

void mbc0_free(IMapper *mapper);

void mbc0_reset(IMapper *mapper);
