#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct Timer Timer;

Timer *timer_new(void);

void timer_free(Timer **t);

void timer_reset(Timer *t);

uint8_t timer_read(Timer *t, uint16_t addr);

void timer_write(Timer *t, uint16_t addr, uint8_t val);

void timer_step(Timer *t);

bool timer_interrupt(Timer *t);
