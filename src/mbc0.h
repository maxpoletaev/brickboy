#pragma once

#include <stdint.h>

#include "rom.h"
#include "mapper.h"

typedef struct {
    ROM *rom;
} MBC0;

const MapperVT mbc0_vtable;

int mbc0_init(Mapper *mapper, ROM *rom);

void mbc0_write(Mapper *mapper, uint16_t addr, uint8_t data);

uint8_t mbc0_read(Mapper *mapper, uint16_t addr);

void mbc0_deinit(Mapper *mapper);

void mbc0_reset(Mapper *mapper);
