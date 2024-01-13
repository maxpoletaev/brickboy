#ifndef BRICKBOY_DISASM_H
#define BRICKBOY_DISASM_H

#include <stdint.h>
#include <stdio.h>

#include "cpu.h"
#include "bus.h"

void disasm_step(Bus *bus, CPU *cpu, FILE *out);

#endif //BRICKBOY_DISASM_H
