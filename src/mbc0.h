#ifndef BRICKBOY_MBC0_H
#define BRICKBOY_MBC0_H

#include <stdint.h>
#include "rom.h"
#include "mapper.h"

typedef struct {
    gb_rom_t *rom;
} gb_mbc0_t;

int gb_mbc0_init(gb_mapper_t *mapper, gb_rom_t *rom);

void gb_mbc0_write(gb_mapper_t *mapper, uint16_t addr, uint8_t data);

uint8_t gb_mbc0_read(gb_mapper_t *mapper, uint16_t addr);

void gb_mbc0_free(gb_mapper_t *mapper);

void gb_mbc0_reset(gb_mapper_t *mapper);

#endif //BRICKBOY_MBC0_H
