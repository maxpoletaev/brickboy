#ifndef BRICKBOY_MAPPER_H
#define BRICKBOY_MAPPER_H

#include <stdint.h>
#include <assert.h>

#include "rom.h"

typedef struct Mapper Mapper;

typedef struct {
    int (*init)(Mapper *mapper, ROM *rom);
    void (*write)(Mapper *mapper, uint16_t addr, uint8_t data);
    uint8_t (*read)(Mapper *mapper, uint16_t addr);
    void (*reset)(Mapper *mapper);
    void (*free)(Mapper *mapper);
} MapperVT;

struct Mapper {
    void *impl;
    uint8_t id;
    const char *name;
    const MapperVT *vt;
};

int mapper_init(Mapper *mapper, ROM *rom);

void mapper_write(Mapper *mapper, uint16_t addr, uint8_t data);

uint8_t mapper_read(Mapper *mapper, uint16_t addr);

void mapper_reset(Mapper *mapper);

void mapper_free(Mapper *mapper);

#endif //BRICKBOY_MAPPER_H
