#ifndef BRICKBOY_MBC0_H
#define BRICKBOY_MBC0_H

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

void mbc0_free(Mapper *mapper);

void mbc0_reset(Mapper *mapper);


#endif //BRICKBOY_MBC0_H
