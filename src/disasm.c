#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "disasm.h"
#include "strbuf.h"
#include "bus.h"
#include "cpu.h"

static inline bool
gb_arg_is_immediate(gb_operand_t arg)
{
    switch (arg) {
    case ARG_IMM8:
    case ARG_IMM16:
    case ARG_IND_IMM8:
    case ARG_IND_IMM16:
        return true;
    default:
        return false;
    }
}

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
gb_format_bytes(gb_strbuf_t *str, gb_bus_t *bus)
{
    uint16_t pc = bus->cpu->pc;
    uint8_t opcode = gb_bus_read(bus, pc++);
    gb_strbuf_addf(str, "0x%02X", opcode);
    const gb_opcode_t *op = &gb_opcodes[opcode];
    int opsize = 1;

    if (opcode == 0xCB) {
        opcode = gb_bus_read(bus, pc++);
        gb_strbuf_addf(str, " 0x%02X", opcode);
        op = &gb_cb_opcodes[opcode];
        opsize = 2;
    }

    opsize += gb_arg_size(op->arg1);
    opsize += gb_arg_size(op->arg2);

    // Instruction operands
    for (int i = 0; i < opsize - 1; i++) {
        gb_strbuf_addf(str, " 0x%02X", gb_bus_read(bus, pc++));
    }
}

static inline void
gb_format_text(gb_strbuf_t *str, gb_bus_t *bus)
{
    uint16_t arg_value;
    uint16_t pc = bus->cpu->pc;
    uint8_t opcode = gb_bus_read(bus, pc++);
    const gb_opcode_t *op = &gb_opcodes[opcode];

    if (opcode == 0xCB) {
        opcode = gb_bus_read(bus, pc++);
        op = &gb_cb_opcodes[opcode];
    }

    if (op->handler == NULL) {
        gb_strbuf_addf(str, "???");
        return;
    }

    if (gb_arg_is_immediate(op->arg1)) {
        arg_value = gb_arg_value(op->arg1, bus, pc);
        gb_strbuf_addf(str, op->mnemonic, arg_value);
    } else if (gb_arg_is_immediate(op->arg2)) {
        arg_value = gb_arg_value(op->arg2, bus, pc);
        gb_strbuf_addf(str, op->mnemonic, arg_value);
    } else {
        gb_strbuf_add(str, op->mnemonic);
    }
}

static inline void
gb_format_cpuflags(gb_strbuf_t *str, gb_bus_t *bus)
{
    gb_cpu_t *cpu = bus->cpu;
    gb_strbuf_addf(str, "[%c %c %c %c]",
                 (cpu->flags.zero ? 'Z' : '-'),
                 (cpu->flags.subtract ? 'N' : '-'),
                 (cpu->flags.half_carry ? 'H' : '-'),
                 (cpu->flags.carry ? 'C' : '-'));
}

static inline void
gb_format_cpustate(gb_strbuf_t *str, gb_bus_t *bus)
{
    gb_cpu_t *cpu = bus->cpu;
    gb_strbuf_addf(str, "AF:%04X BC:%04X DE:%04X HL:%04X SP:%04X CYC:%llu",
                  cpu->af, cpu->bc, cpu->de, cpu->hl, cpu->sp, bus->cycles);
}

void
gb_disasm_step(gb_bus_t *bus, FILE *out)
{
    gb_strbuf_t *str = &(gb_strbuf_t){0};
    gb_strbuf_init(str);

    gb_strbuf_addf(str, "0x%04X: ", bus->cpu->pc);
    gb_strbuf_pad(str, 9, ' ');

    gb_format_bytes(str, bus);
    gb_strbuf_pad(str, 28, ' ');
    gb_format_text(str, bus);
    gb_strbuf_pad(str, 44, ' ');
    gb_format_cpuflags(str, bus);
    gb_strbuf_pad(str, 60, ' ');
    gb_format_cpustate(str, bus);

    fprintf(out, "%s\n", gb_strbuf_data(str));
}
