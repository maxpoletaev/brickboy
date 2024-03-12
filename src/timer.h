#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct Timer {
    uint8_t divider; // DIV ($FF04)
    uint8_t counter; // TIMA ($FF05)
    uint8_t reload;  // TMA ($FF06)
    uint8_t ctrl;    // TAC ($FF07)

    bool interrupt;
    int internal_divider;
    int internal_counter;
} Timer;

Timer *timer_new(void);

void timer_free(Timer **t);

void timer_reset(Timer *t);

uint8_t timer_read(Timer *t, uint16_t addr);

void timer_write(Timer *t, uint16_t addr, uint8_t val);

void timer_step(Timer *t);
