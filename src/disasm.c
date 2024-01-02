#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "disasm.h"
#include "strbuf.h"
#include "bus.h"
#include "cpu.h"

static inline int
gb_arg_size(gb_operand_t arg)
{
    switch (arg) {
    case ARG_IMM8:
    case ARG_IND_IMM8:
        return 1;
    case ARG_IMM16:
    case ARG_IND_IMM16:
        return 2;
    default:
        return 0;
    }
}

static inline uint16_t
gb_arg_value(gb_operand_t arg, gb_bus_t *bus, uint16_t pc)
{
    switch (arg) {
    case ARG_IMM8:
    case ARG_IND_IMM8:
        return gb_bus_read(bus, pc);
    case ARG_IMM16:
    case ARG_IND_IMM16:
        return gb_bus_read16(bus, pc);
    default:
        return 0;
    }
}

static inline void
gb_arg_indirect(gb_strbuf_t *str, gb_operand_t arg, gb_bus_t *bus, gb_cpu_t *cpu)
{
    uint16_t pc = cpu->pc;
    uint16_t addr = 0;
    uint16_t val = 0;

    switch (arg) {
    case ARG_IND_C:
        addr = 0xFF00 + cpu->c;
        val = gb_bus_read(bus, addr);
        gb_strbuf_addf(str, " @ (C)=%02X", val);
        break;
    case ARG_IND_BC:
        addr = cpu->bc;
        val = gb_bus_read(bus, addr);
        gb_strbuf_addf(str, " @ (BC)=%02X", val);
        break;
    case ARG_IND_DE:
        addr = cpu->de;
        val = gb_bus_read(bus, addr);
        gb_strbuf_addf(str, " @ (DE)=%02X", val);
        break;
    case ARG_IND_HL:
    case ARG_IND_HLI:
    case ARG_IND_HLD:
        addr = cpu->hl;
        val = gb_bus_read(bus, addr);
        gb_strbuf_addf(str, " @ (HL)=%02X", val);
        break;
    case ARG_IND_IMM8:
        addr = gb_arg_value(arg, bus, pc+1);
        val = gb_bus_read(bus, addr);
        gb_strbuf_addf(str, " @ ($%02X)=%02X", addr, val);
        break;
    case ARG_IND_IMM16:
        addr = gb_arg_value(arg, bus, pc+1);
        val = gb_bus_read(bus, addr);
        gb_strbuf_addf(str, " @ ($%04X)=%02X", addr, val);
        break;
    default:
        break;
    }
}

static inline void
gb_format_indirect(gb_strbuf_t *str, gb_bus_t *bus, gb_cpu_t *cpu)
{
    uint16_t pc = cpu->pc;
    uint8_t opcode = gb_bus_read(bus, pc++);
    const gb_instr_t *op = &gb_opcodes[opcode];

    if (opcode == 0xCB) {
        opcode = gb_bus_read(bus, pc++);
        op = &gb_cb_opcodes[opcode];
    }

    gb_arg_indirect(str, op->arg1, bus, cpu);
    gb_arg_indirect(str, op->arg2, bus, cpu);
}

static inline void
gb_format_bytes(gb_strbuf_t *str, gb_bus_t *bus, uint16_t pc)
{
    uint8_t opcode = gb_bus_read(bus, pc++);
    gb_strbuf_addf(str, "%02X", opcode);
    const gb_instr_t *op = &gb_opcodes[opcode];
    int opsize = 1;

    if (opcode == 0xCB) {
        opcode = gb_bus_read(bus, pc++);
        gb_strbuf_addf(str, " %02X", opcode);
        op = &gb_cb_opcodes[opcode];
        opsize = 2;
    }

    opsize += gb_arg_size(op->arg1);
    opsize += gb_arg_size(op->arg2);

    // Instruction operands
    for (int i = 0; i < opsize - 1; i++) {
        gb_strbuf_addf(str, " %02X", gb_bus_read(bus, pc++));
    }
}

static inline void
gb_format_text(gb_strbuf_t *str, gb_bus_t *bus, uint16_t pc)
{
    uint16_t arg_value;
    uint8_t opcode = gb_bus_read(bus, pc++);
    const gb_instr_t *op = &gb_opcodes[opcode];

    if (opcode == 0xCB) {
        opcode = gb_bus_read(bus, pc++);
        op = &gb_cb_opcodes[opcode];
    }

    if (op->handler == NULL) {
        gb_strbuf_addf(str, "???");
        return;
    }

    // Instruction text may contain a format specifier, where in instruction
    // containing an immediate operand, the operand value is substituted.
    if (gb_arg_size(op->arg1) > 0) {
        arg_value = gb_arg_value(op->arg1, bus, pc);
        gb_strbuf_addf(str, op->text, arg_value);
    } else if (gb_arg_size(op->arg2) > 0) {
        arg_value = gb_arg_value(op->arg2, bus, pc);
        gb_strbuf_addf(str, op->text, arg_value);
    } else {
        gb_strbuf_add(str, op->text);
    }
}

void
gb_disasm_step(gb_bus_t *bus, gb_cpu_t *cpu, FILE *out)
{
    char str[256] = {0};
    gb_strbuf_t *buf = &(gb_strbuf_t){0};
    gb_strbuf_init(buf, str, sizeof(str));

    // Program counter: 0x0216
    gb_strbuf_addf(buf, "  0x%04X: ", cpu->pc);
    gb_strbuf_pad(buf, 9, ' ');

    // Instruction bytes: 20 FC
    gb_format_bytes(buf, bus, cpu->pc);
    gb_strbuf_pad(buf, 28, ' ');

    // Instruction text: JR NZ,$FC @ (HL)=00
    gb_format_text(buf, bus, cpu->pc);
    gb_format_indirect(buf, bus, cpu);
    gb_strbuf_pad(buf, 54, ' ');

    // CPU flags: [Z N - C]
    gb_strbuf_addf(buf, "[%c %c %c %c]",
                   (cpu->flags.zero ? 'Z' : '-'),
                   (cpu->flags.subtract ? 'N' : '-'),
                   (cpu->flags.half_carry ? 'H' : '-'),
                   (cpu->flags.carry ? 'C' : '-'));
    gb_strbuf_pad(buf, 68, ' ');

    // CPU registers: AF:0000 BC:0000 ...
    gb_strbuf_addf(buf, "AF:%04X BC:%04X DE:%04X HL:%04X SP:%04X CYC:%llu",
                   cpu->af, cpu->bc, cpu->de, cpu->hl, cpu->sp, cpu->cycle);

    fprintf(out, "%s\n", str);
}
