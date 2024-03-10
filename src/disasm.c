#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "disasm.h"
#include "strbuf.h"
#include "mmu.h"
#include "cpu.h"

static inline int
arg_size(ArgType arg)
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
arg_value(ArgType arg, MMU *mmu, uint16_t pc)
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

static inline void
arg_indirect(Strbuf *str, ArgType arg, MMU *mmu, CPU *cpu)
{
    uint16_t pc = cpu->PC;
    uint16_t addr = 0;
    uint16_t val = 0;

    switch (arg) {
    case ARG_IND_C:
        addr = 0xFF00 + cpu->C;
        val = mmu_read(mmu, addr);
        strbuf_addf(str, " @ (C)=%02X", val);
        break;
    case ARG_IND_BC:
        addr = cpu->BC;
        val = mmu_read(mmu, addr);
        strbuf_addf(str, " @ (BC)=%02X", val);
        break;
    case ARG_IND_DE:
        addr = cpu->DE;
        val = mmu_read(mmu, addr);
        strbuf_addf(str, " @ (DE)=%02X", val);
        break;
    case ARG_IND_HL:
    case ARG_IND_HLI:
    case ARG_IND_HLD:
        addr = cpu->HL;
        val = mmu_read(mmu, addr);
        strbuf_addf(str, " @ (HL)=%02X", val);
        break;
    case ARG_IND_8:
        addr = 0xFF00 + arg_value(arg, mmu, pc + 1);
        val = mmu_read(mmu, addr);
        strbuf_addf(str, " @ ($%02X)=%02X", addr, val);
        break;
    case ARG_IND_16:
        addr = arg_value(arg, mmu, pc + 1);
        val = mmu_read(mmu, addr);
        strbuf_addf(str, " @ ($%04X)=%02X", addr, val);
        break;
    default:
        break;
    }
}

static inline void
format_indirect(Strbuf *str, MMU *mmu, CPU *cpu)
{
    uint16_t pc = cpu->PC;
    uint8_t opcode = mmu_read(mmu, pc++);
    const Instruction *op = &opcodes[opcode];

    if (opcode == 0xCB) {
        opcode = mmu_read(mmu, pc++);
        op = &cb_opcodes[opcode];
    }

    arg_indirect(str, op->arg1, mmu, cpu);
    arg_indirect(str, op->arg2, mmu, cpu);
}

static inline void
format_bytes(Strbuf *str, MMU *mmu, uint16_t pc)
{
    uint8_t opcode = mmu_read(mmu, pc++);
    strbuf_addf(str, "%02X", opcode);
    const Instruction *op = &opcodes[opcode];
    int opsize = 1;

    if (opcode == 0xCB) {
        opcode = mmu_read(mmu, pc++);
        strbuf_addf(str, " %02X", opcode);
        op = &cb_opcodes[opcode];
        opsize = 2;
    }

    opsize += arg_size(op->arg1);
    opsize += arg_size(op->arg2);

    // Instruction operands
    for (int i = 0; i < opsize - 1; i++) {
        strbuf_addf(str, " %02X", mmu_read(mmu, pc++));
    }
}

static inline void
format_text(Strbuf *str, MMU *mmu, uint16_t pc)
{
    uint16_t value;
    uint8_t opcode = mmu_read(mmu, pc++);
    const Instruction *op = &opcodes[opcode];

    if (opcode == 0xCB) {
        opcode = mmu_read(mmu, pc++);
        op = &cb_opcodes[opcode];
    }

    if (op->handler == NULL) {
        strbuf_addf(str, "???");
        return;
    }

    // Instruction text may contain a format specifier, where in instruction
    // containing an immediate operand, the operand value is substituted.
    if (arg_size(op->arg1) > 0) {
        value = arg_value(op->arg1, mmu, pc);
        strbuf_addf(str, op->text, value);
    } else if (arg_size(op->arg2) > 0) {
        value = arg_value(op->arg2, mmu, pc);
        strbuf_addf(str, op->text, value);
    } else {
        strbuf_add(str, op->text);
    }
}

void
disasm_step(MMU *mmu, CPU *cpu, Strbuf *buf)
{
    // Program counter: 0x0216
    strbuf_addf(buf, "  0x%04X: ", cpu->PC);
    strbuf_pad(buf, 9, ' ');

    // Instruction bytes: 20 FC
    format_bytes(buf, mmu, cpu->PC);
    strbuf_pad(buf, 28, ' ');

    // Instruction text: JR NZ,$FC @ (HL)=00
    format_text(buf, mmu, cpu->PC);
    format_indirect(buf, mmu, cpu);
    strbuf_pad(buf, 54, ' ');

    // CPU flags: [Z N - C]
    strbuf_addf(buf, "[%c %c %c %c]",
                (cpu->flags.zero ? 'Z' : '-'),
                (cpu->flags.negative ? 'N' : '-'),
                (cpu->flags.half_carry ? 'H' : '-'),
                (cpu->flags.carry ? 'C' : '-'));
    strbuf_pad(buf, 68, ' ');

    // CPU registers: A:00 B:00 C:00 D:00 E:00 H:00 L:00 SP:0000
    strbuf_addf(buf, "A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%04X",
                cpu->A, cpu->F, cpu->B, cpu->C, cpu->D, cpu->E, cpu->H, cpu->L, cpu->SP);

    // PPU registers
    strbuf_addf(buf, " LCDC:%02X STAT:%02X SCY:%02X SCX:%02X LY:%02X LYC:%02X",
                mmu->ppu.LCDC.raw, mmu->ppu.STAT.raw, mmu->ppu.SCY, mmu->ppu.SCX, mmu->ppu.LY, mmu->ppu.LYC);
}
