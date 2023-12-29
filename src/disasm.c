#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "common.h"
#include "disasm.h"
#include "strbuf.h"
#include "bus.h"
#include "cpu.h"

extern const gb_opcode_t gb_opcodes[256];
extern const gb_opcode_t gb_cb_opcodes[256];

static void
gb_disasm_arg(gb_strbuf_t *str, gb_bus_t *bus, gb_operand_t arg, uint16_t *pc)
{
    switch (arg) {
    case ARG_NONE:
        break;
    case ARG_IMM8:
        gb_strbuf_addf(str, " 0x%02X", gb_bus_read(bus, *pc));
        *pc += 1;
        break;
    case ARG_IMM16:
        gb_strbuf_addf(str, " 0x%04X", gb_bus_read16(bus, *pc));
        *pc += 2;
        break;
    case ARG_REG_A:
        gb_strbuf_addf(str, " A");
        break;
    case ARG_REG_B:
        gb_strbuf_addf(str, " B");
        break;
    default:
        break;
    }
}

static int
gb_disasm_argsize(gb_operand_t arg)
{
    switch (arg) {
    case ARG_IMM8:
        return 1;
    case ARG_IMM16:
        return 2;
    default:
        return 0;
    }
}

static void
gb_disasm_raw(gb_strbuf_t *str, gb_bus_t *bus)
{
    uint16_t pc = bus->cpu->pc;
    uint8_t opcode = gb_bus_read(bus, pc++);
    gb_strbuf_addf(str, " 0x%02X", opcode);
    const gb_opcode_t *op = &gb_opcodes[opcode];
    int opsize = 1;

    if (opcode == 0xCB) {
        opcode = gb_bus_read(bus, pc++);
        gb_strbuf_addf(str, " 0x%02X", opcode);
        op = &gb_cb_opcodes[opcode];
        opsize = 2;
    }

    opsize += gb_disasm_argsize(op->target);
    opsize += gb_disasm_argsize(op->source);

    // Instruction operands
    for (int i = 0; i < opsize - 1; i++) {
        gb_strbuf_addf(str, " 0x%02X ", gb_bus_read(bus, pc++));
    }
}

static void
gb_disasm_mnemonic(gb_strbuf_t *str, gb_bus_t *bus)
{
    uint16_t pc = bus->cpu->pc;
    uint8_t opcode = gb_bus_read(bus, pc++);
    const gb_opcode_t *op = &gb_opcodes[opcode];

    if (opcode == 0xCB) {
        opcode = gb_bus_read(bus, pc++);
        op = &gb_cb_opcodes[opcode];
    }

    if (op->handler != NULL) {
        pc = bus->cpu->pc;
        gb_strbuf_addf(str, "%s", op->mnemonic);
        gb_disasm_arg(str, bus, op->target, &pc);
        gb_disasm_arg(str, bus, op->source, &pc);
    } else {
        gb_strbuf_addf(str, "???");
    }
}

static void
gb_disasm_cpuflags(gb_strbuf_t *str, gb_bus_t *bus)
{
    gb_cpu_t *cpu = bus->cpu;
    gb_strbuf_addf(str, "[%c %c %c %c]",
                 (cpu->flags.zero ? 'Z' : 'z'),
                 (cpu->flags.subtract ? 'N' : 'n'),
                 (cpu->flags.half_carry ? 'H' : 'h'),
                 (cpu->flags.carry ? 'C' : 'c'));
}

static void
gb_disasm_cpustate(gb_strbuf_t *str, gb_bus_t *bus)
{
    gb_cpu_t *cpu = bus->cpu;
    gb_strbuf_addf(str, "CYC:%llu, AF:%04X, BC:%04X, DE:%04X, HL:%04X, SP:%04X",
                  bus->cycles, cpu->af, cpu->bc, cpu->de, cpu->hl, cpu->sp);
}

void
gb_disasm_step(gb_bus_t *bus)
{
    gb_strbuf_t *str = &(gb_strbuf_t){0};
    gb_strbuf_init(str);

    gb_strbuf_addf(str, "0x%04X:", bus->cpu->pc);

    gb_disasm_raw(str, bus);
    gb_strbuf_pad(str, 20, ' ');
    gb_disasm_mnemonic(str, bus);
    gb_strbuf_pad(str, 32, ' ');
    gb_disasm_cpuflags(str, bus);
    gb_strbuf_pad(str, 48, ' ');
    gb_disasm_cpustate(str, bus);

    fprintf(stdout, "%s\n", gb_strbuf_data(str));
}
