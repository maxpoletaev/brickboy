#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "mapper.h"

typedef struct MBC1 {
    IMapper imapper;
    uint8_t mode;
    ROM *rom;
    bool ram_enabled;
    uint32_t ram_size;
    uint8_t *ram;
    uint8_t rom_bank;
    uint8_t ram_bank;
} MBC1;

IMapper *mbc1_new(ROM *rom);

void mbc1_write(IMapper *mapper, uint16_t addr, uint8_t data);

uint8_t mbc1_read(IMapper *mapper, uint16_t addr);

void mbc1_free(IMapper *mapper);

void mbc1_reset(IMapper *mapper);
