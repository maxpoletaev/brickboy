#ifndef BRICKBOY_MAPPER_H
#define BRICKBOY_MAPPER_H

#include <stdint.h>
#include <assert.h>
#include "rom.h"

typedef struct gb_mapper_t gb_mapper_t;

typedef struct {
    int (*init)(gb_mapper_t *mapper, gb_rom_t *rom);
    void (*write)(gb_mapper_t *mapper, uint16_t addr, uint8_t data);
    uint8_t (*read)(gb_mapper_t *mapper, uint16_t addr);
    void (*reset)(gb_mapper_t *mapper);
    void (*free)(gb_mapper_t *mapper);
} gb_mapper_vt;

struct gb_mapper_t {
    void *impl;
    const char *name;
    const gb_mapper_vt *vt;
};

int gb_mapper_init(gb_mapper_t *mapper, gb_rom_t *rom);

void gb_mapper_write(gb_mapper_t *mapper, uint16_t addr, uint8_t data);

uint8_t gb_mapper_read(gb_mapper_t *mapper, uint16_t addr);

void gb_mapper_reset(gb_mapper_t *mapper);

void gb_mapper_free(gb_mapper_t *mapper);

#endif //BRICKBOY_MAPPER_H
