#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "disasm.h"
#include "mmu.h"
#include "cpu.h"
#include "str.h"

static inline int
disasm_arg_size(ArgType arg)
{
    switch (arg) {
    case ARG_IMM_8:
    case ARG_IND_8:
        return 1;
    case ARG_IMM_16:
    case ARG_IND_16:
        return 2;
    default:
        return 0;
    }
}

static inline uint16_t
disasm_arg_value(ArgType arg, MMU *mmu, uint16_t pc)
{
    switch (arg) {
    case ARG_IMM_8:
    case ARG_IND_8:
        return mmu_read(mmu, pc);
    case ARG_IMM_16:
    case ARG_IND_16:
        return mmu_read16(mmu, pc);
    default:
        return 0;
    }
}

static inline String
disasm_add_memval(String str, ArgType arg, MMU *mmu, CPU *cpu)
{
    uint16_t pc = cpu->PC;
    uint16_t addr = 0;
    uint16_t val = 0;

    switch (arg) {
    case ARG_IND_C:
        addr = 0xFF00 + cpu->C;
        val = mmu_read(mmu, addr);
        str = str_addf(str, " @ (C)=%02X", val);
        break;
    case ARG_IND_BC:
        addr = cpu->BC;
        val = mmu_read(mmu, addr);
        str = str_addf(str, " @ (BC)=%02X", val);
        break;
    case ARG_IND_DE:
        addr = cpu->DE;
        val = mmu_read(mmu, addr);
        str = str_addf(str, " @ (DE)=%02X", val);
        break;
    case ARG_IND_HL:
    case ARG_IND_HLI:
    case ARG_IND_HLD:
        addr = cpu->HL;
        val = mmu_read(mmu, addr);
        str = str_addf(str, " @ (HL)=%02X", val);
        break;
    case ARG_IND_8:
        val = mmu_read(mmu, addr);
        addr = 0xFF00 + disasm_arg_value(arg, mmu, pc + 1);
        str = str_addf(str, " @ ($%02X)=%02X", addr, val);
        break;
    case ARG_IND_16:
        val = mmu_read(mmu, addr);
        addr = disasm_arg_value(arg, mmu, pc + 1);
        str = str_addf(str, " @ ($%04X)=%02X", addr, val);
        break;
    default:
        break;
    }

    return str;
}

static inline String
disasm_format_bytes(String str, MMU *mmu, uint16_t pc)
{
    uint8_t opcode = mmu_read(mmu, pc++);
    str = str_addf(str, "%02X", opcode);

    const Instruction *op = &opcodes[opcode];
    int opsize = 1;

    if (opcode == 0xCB) {
        opcode = mmu_read(mmu, pc++);
        str = str_addf(str, " %02X", opcode);
        op = &cb_opcodes[opcode];
        opsize = 2;
    }

    opsize += disasm_arg_size(op->arg1);
    opsize += disasm_arg_size(op->arg2);

    // Instruction operands
    for (int i = 0; i < opsize - 1; i++) {
        str = str_addf(str, " %02X", mmu_read(mmu, pc++));
    }

    return str;
}

static inline String
disasm_format_text(String str, MMU *mmu, CPU *cpu)
{
    uint16_t value;
    uint16_t pc = cpu->PC;
    uint8_t opcode = mmu_read(mmu, pc++);
    const Instruction *op = &opcodes[opcode];

    if (opcode == 0xCB) {
        opcode = mmu_read(mmu, pc++);
        op = &cb_opcodes[opcode];
    }

    if (op->handler == NULL) {
        str = str_add(str, "???");
        return str;
    }

    // Instruction text may contain a format specifier, where in instruction
    // containing an immediate operand, the operand value is substituted.
    if (disasm_arg_size(op->arg1) > 0) {
        value = disasm_arg_value(op->arg1, mmu, pc);
        str = str_addf(str, op->text, value);
    } else if (disasm_arg_size(op->arg2) > 0) {
        value = disasm_arg_value(op->arg2, mmu, pc);
        str = str_addf(str, op->text, value);
    } else {
        str = str_add(str, op->text);
    }

    // Indirect memory value: @ (HL)=00
    str = disasm_add_memval(str, op->arg1, mmu, cpu);
    str = disasm_add_memval(str, op->arg2, mmu, cpu);

    return str;
}

String
disasm_step(MMU *mmu, CPU *cpu, String str)
{
    // Program counter: 0x0216
    str = str_addf(str, "  0x%04X: ", cpu->PC);
    str = str_pad(str, 9, ' ');

    // Instruction bytes: 20 FC
    str = disasm_format_bytes(str, mmu, cpu->PC);
    str = str_pad(str, 28, ' ');

    // Instruction text: JR NZ,$FC @ (HL)=00
    str = disasm_format_text(str, mmu, cpu);
    str = str_pad(str, 54, ' ');

    // CPU flags: [Z N - C]
    str = str_addf(str, "[%c %c %c %c]",
                    (cpu->flags.zero ? 'Z' : '-'),
                    (cpu->flags.negative ? 'N' : '-'),
                    (cpu->flags.half_carry ? 'H' : '-'),
                    (cpu->flags.carry ? 'C' : '-'));
    str = str_pad(str, 68, ' ');

    // CPU registers: A:00 B:00 C:00 D:00 E:00 H:00 L:00 SP:0000
    str = str_addf(str, "A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%04X",
                    cpu->A, cpu->F, cpu->B, cpu->C, cpu->D, cpu->E, cpu->H, cpu->L, cpu->SP);

    return str;
}
