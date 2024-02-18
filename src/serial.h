#ifndef BRICKBOY_SERIAL_H
#define BRICKBOY_SERIAL_H

#include <stdint.h>
#include "common.h"

typedef struct {
    uint8_t byte;

    union {
        uint8_t ctrl;
        struct {
            uint8_t start: 1;
            uint8_t unused_: 6;
            uint8_t transfer: 1;
        } PACKED;
    };
} Serial;

void serial_reset(Serial *s);

uint8_t serial_read(Serial *s, uint16_t addr);

void serial_write(Serial *s, uint16_t addr, uint8_t val);

void serial_print(Serial *s);

#endif //BRICKBOY_SERIAL_H
