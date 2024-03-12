#include <stdbool.h>
#include <stdint.h>

#include "common.h"
#include "timer.h"

static const int timer_freqs[] = {1024, 16, 64, 256};

struct Timer {
    uint8_t divider; // DIV ($FF04)
    uint8_t counter; // TIMA ($FF05)
    uint8_t reload;  // TMA ($FF06)
    uint8_t ctrl;    // TAC ($FF07)

    bool interrupt;
    int internal_divider;
    int internal_counter;
};

Timer *
timer_new(void)
{
    Timer *t = xalloc(sizeof(Timer));
    timer_reset(t);
    return t;
}

void
timer_free(Timer **t)
{
    xfree(*t);
}

void
timer_reset(Timer *t)
{
    t->divider = 0;
    t->counter = 0;
    t->reload = 0;
    t->ctrl = 0;
    t->interrupt = false;
    t->internal_counter = 0;
    t->internal_divider = 0;
}

uint8_t
timer_read(Timer *t, uint16_t addr)
{
    switch (addr) {
    case 0xFF04:
        return t->divider;
    case 0xFF05:
        return t->counter;
    case 0xFF06:
        return t->reload;
    case 0xFF07:
        return t->ctrl;
    default:
        PANIC("unhandled timer read at 0x%04X", addr);
    }
}

void
timer_write(Timer *t, uint16_t addr, uint8_t val)
{
    switch (addr) {
    case 0xFF04:
        t->divider = 0;
        break;
    case 0xFF05:
        t->counter = val;
        break;
    case 0xFF06:
        t->reload = val;
        break;
    case 0xFF07:
        t->ctrl = val;
        break;
    default:
        PANIC("unhandled timer write at 0x%04X", addr);
    }
}

void
timer_step(Timer *t)
{
    bool enabled = (t->ctrl & (1 << 2)) != 0;

    t->internal_divider++;
    if (t->internal_divider == 256) {
        t->internal_divider = 0;
        t->divider++;
    }

    if (enabled) {
        int freq = timer_freqs[t->ctrl & 0x03];
        t->internal_counter++;

        if (t->internal_counter == freq) {
            t->internal_counter = 0;
            t->counter++;

            if (t->counter == 0xFF) {
                t->interrupt = true;
            } else if (t->counter == 0x00) {
                t->counter = t->reload;
            }
        }
    }
}

inline bool
timer_interrupt(Timer *t)
{
    if (t->interrupt) {
        t->interrupt = false;
        return true;
    }

    return false;
}
