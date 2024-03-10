#pragma once

#include <stdint.h>
#include "common.h"

typedef struct Serial {
    uint8_t byte;

    union {
        uint8_t ctrl;
        struct {
            uint8_t master: 1;
            uint8_t double_speed: 1; // GCB only
            uint8_t _unused_: 5;
            uint8_t transfer: 1;
        } _packed_;
    };
} Serial;

void serial_reset(Serial *s);

uint8_t serial_read(Serial *s, uint16_t addr);

void serial_write(Serial *s, uint16_t addr, uint8_t val);

void serial_print(Serial *s);
