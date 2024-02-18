#ifndef BRICKBOY_DISASM_H
#define BRICKBOY_DISASM_H

#include <stdint.h>
#include <stdio.h>

#include "cpu.h"
#include "mmu.h"

void disasm_step(MMU *mmu, CPU *cpu, FILE *out);

#endif //BRICKBOY_DISASM_H
