#pragma once

#include <stdint.h>
#include <stdio.h>

#include "cpu.h"
#include "mmu.h"
#include "strbuf.h"

void disasm_step(MMU *mmu, CPU *cpu, Strbuf *buf);
