#include <ctype.h>
#include <stdint.h>
#include <stdio.h>

#include "common.h"
#include "serial.h"

Serial *
serial_new(void)
{
    Serial *s = xalloc(sizeof(Serial));
    serial_reset(s);
    return s;
}

void serial_free(Serial **s)
{
    xfree(*s);
}

void
serial_reset(Serial *s)
{
    s->byte = 0;
    s->ctrl = 0;
}

uint8_t
serial_read(Serial *s, uint16_t addr)
{
    switch (addr) {
    case 0xFF01:
        return s->byte;
    case 0xFF02:
        return s->ctrl;
    default:
        PANIC("unhandled serial read at 0x%04X", addr);
    }
}

void
serial_write(Serial *s, uint16_t addr, uint8_t val)
{
    switch (addr) {
    case 0xFF01:
        s->byte = val;
        break;
    case 0xFF02:
        s->ctrl = val;
        break;
    default:
        PANIC("unhandled serial write at 0x%04X", addr);
    }
}

void
serial_print(Serial *s) {
    if (s->transfer) {
        if (isprint(s->byte) || s->byte == '\n' || s->byte == '\r' || s->byte == '\t') {
            putchar(s->byte);
        }

        s->transfer = 0;
    }
}
