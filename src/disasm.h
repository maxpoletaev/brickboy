#ifndef BRICKBOY_DISASM_H
#define BRICKBOY_DISASM_H

#include <stdint.h>
#include <stdio.h>

#include "cpu.h"
#include "bus.h"

void gb_disasm_step(gb_bus_t *bus, gb_cpu_t *cpu, FILE *out);

#endif //BRICKBOY_DISASM_H
