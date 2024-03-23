#pragma once

#include <stdint.h>

#include "rom.h"
#include "mapper.h"

typedef struct {
    IMapper imapper;
    ROM *rom;
} MBC0;

IMapper *mbc0_new(ROM *ROM);

void mbc0_write(IMapper *mapper, uint16_t addr, uint8_t data);

uint8_t mbc0_read(IMapper *mapper, uint16_t addr);

void mbc0_free(IMapper *mapper);

void mbc0_reset(IMapper *mapper);

int mbc0_save(IMapper *mapper, const char *filename);

int mbc0_load(IMapper *mapper, const char *filename);
