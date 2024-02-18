#ifndef BRICKBOY_TIMER_H
#define BRICKBOY_TIMER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t divider; // DIV ($FF04)
    uint8_t counter; // TIMA ($FF05)
    uint8_t reload;  // TMA ($FF06)
    uint8_t ctrl;    // TAC ($FF07)

    bool interrupt;
    int internal_divider;
    int internal_counter;

} Timer;

void timer_reset(Timer *t);

uint8_t timer_read(Timer *t, uint16_t addr);

void timer_write(Timer *t, uint16_t addr, uint8_t val);

void timer_step_fast(Timer *timer, int cycles);

#endif //BRICKBOY_TIMER_H
