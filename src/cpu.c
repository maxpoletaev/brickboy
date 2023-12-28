#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "cpu.h"
#include "bus.h"

void
gb_cpu_reset(gb_cpu_t *cpu)
{
    cpu->af = 0;
    cpu->bc = 0;
    cpu->de = 0;
    cpu->hl = 0;
    cpu->sp = 0;
    cpu->pc = 0;
    cpu->halt = 0;
}

/*
 * Read an 8-bit value from the CPU.
 * That includes registers, memory locations, and immediate values.
 */
static uint16_t
gb_cpu_get(gb_cpu_t *cpu, gb_bus_t *bus, gb_operand_t src)
{
    uint8_t value;
    uint8_t addr;

    switch (src) {
    case ARG_REG_A:
        value = cpu->a;
        break;
    case ARG_REG_B:
        value = cpu->b;
        break;
    case ARG_REG_C:
        value = cpu->c;
        break;
    case ARG_REG_D:
        value = cpu->d;
        break;
    case ARG_REG_E:
        value = cpu->e;
        break;
    case ARG_REG_H:
        value = cpu->h;
        break;
    case ARG_REG_L:
        value = cpu->l;
        break;
    case ARG_IND_BC:
        value = gb_bus_read(bus, cpu->bc);
        break;
    case ARG_IND_DE:
        value = gb_bus_read(bus, cpu->de);
        break;
    case ARG_IND_HL:
        value = gb_bus_read(bus, cpu->hl);
        break;
    case ARG_IND_HLI:
        value = gb_bus_read(bus, cpu->hl++);
        break;
    case ARG_IND_HLD:
        value = gb_bus_read(bus, cpu->hl--);
        break;
    case ARG_IMM8:
        value = gb_bus_read(bus, cpu->pc++);
        break;
    case ARG_IND8:
        addr = gb_bus_read(bus, cpu->pc++);
        value = gb_bus_read(bus, addr);
        break;
    case ARG_IND16:
        addr = gb_bus_read(bus, cpu->pc++);
        addr |= gb_bus_read(bus, cpu->pc++) << 8;
        value = gb_bus_read(bus, addr);
        break;
    default:
        GB_TRACE("unhandled operand: %d", src);
        GB_UNREACHABLE();
    }

    return value;
}

static void
gb_cpu_set(gb_cpu_t *cpu, gb_bus_t *bus, gb_operand_t target, uint16_t value)
{
    uint8_t addr;

    switch (target) {
    case ARG_REG_A:
        cpu->a = value;
        break;
    case ARG_REG_B:
        cpu->b = value;
        break;
    case ARG_REG_C:
        cpu->c = value;
        break;
    case ARG_REG_D:
        cpu->d = value;
        break;
    case ARG_REG_E:
        cpu->e = value;
        break;
    case ARG_REG_H:
        cpu->h = value;
        break;
    case ARG_REG_L:
        cpu->l = value;
        break;
    case ARG_REG_AF:
        value = cpu->af;
        break;
    case ARG_REG_BC:
        value = cpu->bc;
        break;
    case ARG_REG_DE:
        value = cpu->de;
        break;
    case ARG_REG_HL:
        value = cpu->hl;
    case ARG_REG_SP:
        value = cpu->sp;
    case ARG_IND_BC:
        gb_bus_write(bus, cpu->bc, value);
        break;
    case ARG_IND_DE:
        gb_bus_write(bus, cpu->de, value);
        break;
    case ARG_IND_HL:
        gb_bus_write(bus, cpu->hl, value);
        break;
    case ARG_IND_HLI:
        gb_bus_write(bus, cpu->hl++, value);
        break;
    case ARG_IND_HLD:
        gb_bus_write(bus, cpu->hl--, value);
        break;
    case ARG_IND8:
        addr = gb_bus_read(bus, cpu->pc++);
        gb_bus_write(bus, addr, value);
        break;
    case ARG_IND16:
        addr = gb_bus_read(bus, cpu->pc++);
        addr |= gb_bus_read(bus, cpu->pc++) << 8;
        gb_bus_write(bus, addr, value);
        break;
    case ARG_IMM16:
        value = gb_bus_read(bus, cpu->pc++);
        value |= gb_bus_read(bus, cpu->pc++) << 8;
        break;
    default:
        GB_TRACE("unhandled operand: %d", target);
        GB_UNREACHABLE();
    }
}

/*
 * NOP
 * Flags: - - - -
 * No operation.
 */
static void
gb_nop(gb_cpu_t *cpu, gb_bus_t *bus, const gb_opcode_t *op)
{
    // Do nothing.
}

/*
 * INC r8
 * Flags: Z 0 H -
 * Increment 8-bit register or memory location.
 */
static void
gb_inc_r8(gb_cpu_t *cpu, gb_bus_t *bus, const gb_opcode_t *op)
{
    uint8_t v = gb_cpu_get(cpu, bus, op->target);
    uint8_t r = v + 1;

    cpu->flags.subtract = 0;
    cpu->flags.zero = r == 0;
    cpu->flags.half_carry = (r & 0x0F) == 0;
    gb_cpu_set(cpu, bus, op->target, r);
}

#define GB_OPCODE(op, source, target, handler, mnemonic) \
    [op] = {op, 1, source, target, handler, mnemonic}

static const
gb_opcode_t gb_opcodes[0xFF] = {
    GB_OPCODE(0x00, ARG_NONE, ARG_NONE, gb_nop, "NOP"),   // NOP
    GB_OPCODE(0x04, ARG_REG_B, ARG_NONE, gb_inc_r8, "INC"), // INC B
};

static const
gb_opcode_t gb_cb_opcodes[0xFF] = {
    GB_OPCODE(0x00, ARG_NONE, ARG_NONE, gb_nop, "NOP"),   // NOP
};

void
gb_cpu_step(gb_cpu_t *cpu, gb_bus_t *bus)
{
    cpu->cycles++;

    if (cpu->halt > 0) {
        cpu->halt--;
        return;
    }

    uint8_t opcode = gb_bus_read(bus, cpu->pc++);
    const gb_opcode_t *op = &gb_opcodes[opcode];

    // Prefixed instructions
    if (opcode == 0xCB) {
        opcode = gb_bus_read(bus, cpu->pc++);
        op = &gb_cb_opcodes[opcode];
    }

    if (op->handler == NULL) {
        GB_TRACE("invalid opcode 0x%02X", opcode);
        abort();
    }

    cpu->halt = op->cycles - 1;

    op->handler(cpu, bus, op);
}
