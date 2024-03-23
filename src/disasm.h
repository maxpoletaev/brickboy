#pragma once

#include <stdint.h>
#include <stdio.h>

#include "cpu.h"
#include "mmu.h"
#include "str.h"

String disasm_step(MMU *mmu, CPU *cpu, String str);
