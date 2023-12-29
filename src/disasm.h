#ifndef BRICKBOY_DISASM_H
#define BRICKBOY_DISASM_H

#include <stdint.h>

#include "cpu.h"
#include "bus.h"

void gb_disasm_step(gb_bus_t *bus);

#endif //BRICKBOY_DISASM_H
