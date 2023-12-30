#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "shared.h"
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
    cpu->cycles = 0;
}

static uint16_t
gb_cpu_get(gb_cpu_t *cpu, gb_bus_t *bus, gb_operand_t src)
{
    uint16_t value = 0;
    uint16_t addr = 0;

    switch (src) {
    case ARG_NONE:
        break;
    case ARG_IMM16:
        value = (uint16_t) (gb_bus_read(bus, cpu->pc++) << 0);
        value |= (uint16_t) (gb_bus_read(bus, cpu->pc++) << 8);
        break;
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
        break;
    case ARG_REG_SP:
        value = cpu->sp;
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
    case ARG_IND_IMM8:
        addr = gb_bus_read(bus, cpu->pc++);
        value = gb_bus_read(bus, addr);
        break;
    case ARG_IND_IMM16:
        addr = (uint16_t) (gb_bus_read(bus, cpu->pc++) << 0);
        addr |= (uint16_t) (gb_bus_read(bus, cpu->pc++) << 8);
        value = gb_bus_read(bus, addr);
        break;
    }

    return value;
}

static void
gb_cpu_set(gb_cpu_t *cpu, gb_bus_t *bus, gb_operand_t target, uint16_t value16)
{
    uint8_t value8 = (uint8_t) value16;
    uint16_t addr = 0;

    switch (target) {
    case ARG_NONE:
    case ARG_IMM8:
    case ARG_IMM16:
        break;
    case ARG_REG_A:
        cpu->a = value8;
        break;
    case ARG_REG_B:
        cpu->b = value8;
        break;
    case ARG_REG_C:
        cpu->c = value8;
        break;
    case ARG_REG_D:
        cpu->d = value8;
        break;
    case ARG_REG_E:
        cpu->e = value8;
        break;
    case ARG_REG_H:
        cpu->h = value8;
        break;
    case ARG_REG_L:
        cpu->l = value8;
        break;
    case ARG_REG_AF:
        cpu->af = value16;
        break;
    case ARG_REG_BC:
        cpu->bc = value16;
        break;
    case ARG_REG_DE:
        cpu->de = value16;
        break;
    case ARG_REG_HL:
        cpu->hl = value16;
    case ARG_REG_SP:
        cpu->sp = value16;
        break;
    case ARG_IND_BC:
        gb_bus_write(bus, cpu->bc, value8);
        break;
    case ARG_IND_DE:
        gb_bus_write(bus, cpu->de, value8);
        break;
    case ARG_IND_HL:
        gb_bus_write(bus, cpu->hl, value8);
        break;
    case ARG_IND_HLI:
        gb_bus_write(bus, cpu->hl++, value8);
        break;
    case ARG_IND_HLD:
        gb_bus_write(bus, cpu->hl--, value8);
        break;
    case ARG_IND_IMM8:
        addr = gb_bus_read(bus, cpu->pc++);
        gb_bus_write(bus, addr, value8);
        break;
    case ARG_IND_IMM16:
        addr = (uint16_t) (gb_bus_read(bus, cpu->pc++) << 0);
        addr |= (uint16_t) (gb_bus_read(bus, cpu->pc++) << 8);
        gb_bus_write(bus, addr, value8);
        break;
    }
}

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

static bool
gb_is_arg_8bit(gb_operand_t arg)
{
    if (arg == ARG_NONE)
        GB_PANIC("invalid argument");

    switch (arg) {
    case ARG_REG_BC:
    case ARG_REG_DE:
    case ARG_REG_HL:
    case ARG_REG_SP:
    case ARG_REG_AF:
    case ARG_IMM16:
        return false;
    default:
        return true;
    }
}

/* ----------------------------------------------------------------------------
 * Instruction Handlers
 * -------------------------------------------------------------------------- */

/* NOP
 * Flags: - - - -
 * No operation. */
static void
gb_nop(gb_cpu_t *cpu, gb_bus_t *bus, const gb_opcode_t *op)
{
    GB_UNUSED(cpu);
    GB_UNUSED(bus);
    GB_UNUSED(op);
}

/* LD r8,r8
 * Flags: - - - -
 * Load 8-bit register or memory location into another 8-bit register or memory location. */
static void
gb_ld_u8(gb_cpu_t *cpu, gb_bus_t *bus, const gb_opcode_t *op)
{
    assert(gb_is_arg_8bit(op->arg1));

    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg2);
    gb_cpu_set(cpu, bus, op->arg1, v);
}

/* LD r16,r16
 * Flags: - - - -
 * Load 16-bit data into 16-bit register. */
static void
gb_ld_u16(gb_cpu_t *cpu, gb_bus_t *bus, const gb_opcode_t *op)
{
    assert(!gb_is_arg_8bit(op->arg1));

    uint16_t v = gb_cpu_get(cpu, bus, op->arg2);
    gb_cpu_set(cpu, bus, op->arg1, v);
}

/* INC r8
 * Flags: Z 0 H -
 * Increment 8-bit register or memory location. */
static void
gb_inc_u8(gb_cpu_t *cpu, gb_bus_t *bus, const gb_opcode_t *op)
{
    assert(gb_is_arg_8bit(op->arg1));

    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t r = v + 1;

    cpu->flags.zero = r == 0;
    cpu->flags.subtract = 0;
    cpu->flags.half_carry = (r&0xF) < (v&0xF);

    gb_cpu_set(cpu, bus, op->arg1, r);
}

/* INC r16
 * Flags: - - - -
 * Increment 16-bit register. */
static void
gb_inc_u16(gb_cpu_t *cpu, gb_bus_t *bus, const gb_opcode_t *op)
{
    assert(!gb_is_arg_8bit(op->arg1));

    uint16_t v = gb_cpu_get(cpu, bus, op->arg1);
    uint16_t r = v + 1;

    gb_cpu_set(cpu, bus, op->arg1, r);
}

/* ADD r8,r8
 * Flags: Z 0 H C
 * Add 8-bit register or memory location to A. */
static void
gb_add_u8(gb_cpu_t *cpu, gb_bus_t *bus, const gb_opcode_t *op)
{
    assert(op->arg1 == ARG_REG_A);
    assert(gb_is_arg_8bit(op->arg2));

    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg2);
    uint8_t a = cpu->a;
    uint8_t r = a + v;

    cpu->flags.zero = r == 0;
    cpu->flags.subtract = 0;
    cpu->flags.half_carry = (r&0xF) < (v&0xF);
    cpu->flags.carry = r < v;

    cpu->a = r;
}

/* ADC r8,r8
 * Flags: Z 0 H C
 * Add 8-bit register or memory location and carry flag to A. */
static void
gb_adc_u8(gb_cpu_t *cpu, gb_bus_t *bus, const gb_opcode_t *op)
{
    assert(op->arg1 == ARG_REG_A);
    assert(gb_is_arg_8bit(op->arg2));

    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg2);
    uint8_t c = cpu->flags.carry;
    uint8_t a = cpu->a;
    uint8_t r = a + v + c;

    cpu->flags.zero = r == 0;
    cpu->flags.subtract = 0;
    cpu->flags.half_carry = (r&0xF) < (v&0xF);
    cpu->flags.carry = r < v;

    cpu->a = r;
}

/* SUB r8,r8
 * Flags: Z 1 H C
 * Subtract 8-bit register or memory location from A. */
static void
gb_sub_u8(gb_cpu_t *cpu, gb_bus_t *bus, const gb_opcode_t *op)
{
    assert(op->arg1 == ARG_REG_A);
    assert(gb_is_arg_8bit(op->arg2));

    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg2);
    uint8_t a = cpu->a;
    uint8_t r = a - v;

    cpu->flags.zero = r == 0;
    cpu->flags.subtract = 1;
    cpu->flags.half_carry = (r&0xF) > (a&0xF);
    cpu->flags.carry = r > a;

    cpu->a = r;
}

/* SBC r8,r8
 * Flags: Z 1 H C
 * Subtract 8-bit register or memory location and carry flag from A. */
static void
gb_sbc_u8(gb_cpu_t *cpu, gb_bus_t *bus, const gb_opcode_t *op)
{
    assert(op->arg1 == ARG_REG_A);
    assert(gb_is_arg_8bit(op->arg2));

    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg2);
    uint8_t c = cpu->flags.carry;
    uint8_t a = cpu->a;
    uint8_t r = a - v - c;

    cpu->flags.zero = r == 0;
    cpu->flags.subtract = 1;
    cpu->flags.half_carry = (r&0xF) > (a&0xF);
    cpu->flags.carry = r > a;

    cpu->a = r;
}

/* JP a16
 * Flags: - - - -
 * Jump to 16-bit address provided by immediate operand or register. */
static void gb_jp_a16(gb_cpu_t *cpu, gb_bus_t *bus, const gb_opcode_t *op)
{
    uint16_t addr = gb_cpu_get(cpu, bus, op->arg1);
    cpu->pc = addr;
}

/* XOR r8
 * Flags: Z 0 0 0
 * XOR 8-bit register or memory location with A. */
static void gb_xor_u8(gb_cpu_t *cpu, gb_bus_t *bus, const gb_opcode_t *op)
{
    assert(gb_is_arg_8bit(op->arg1));

    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t a = cpu->a;
    uint8_t r = a ^ v;

    cpu->flags.zero = r == 0;
    cpu->flags.subtract = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = 0;

    cpu->a = r;
}

/* ----------------------------------------------------------------------------
 * Opcode Tables
 * -------------------------------------------------------------------------- */

#define GB_OPCODE(op, arg1, arg2, handler, cost, text) \
    [op] = {op, cost, arg1, arg2, handler, text}

const gb_opcode_t gb_opcodes[256] = {
    GB_OPCODE(0x00, ARG_NONE, ARG_NONE, gb_nop, 1, "NOP"),

    GB_OPCODE(0x03, ARG_REG_BC, ARG_NONE, gb_inc_u16, 2, "INC BC"),
    GB_OPCODE(0x13, ARG_REG_DE, ARG_NONE, gb_inc_u16, 2, "INC DE"),
    GB_OPCODE(0x23, ARG_REG_HL, ARG_NONE, gb_inc_u16, 2, "INC HL"),
    GB_OPCODE(0x33, ARG_REG_SP, ARG_NONE, gb_inc_u16, 2, "INC SP"),

    GB_OPCODE(0x04, ARG_REG_B, ARG_NONE, gb_inc_u8, 1, "INC B"),
    GB_OPCODE(0x0C, ARG_REG_C, ARG_NONE, gb_inc_u8, 1, "INC C"),
    GB_OPCODE(0x14, ARG_REG_D, ARG_NONE, gb_inc_u8, 1, "INC D"),
    GB_OPCODE(0x1C, ARG_REG_E, ARG_NONE, gb_inc_u8, 1, "INC E"),
    GB_OPCODE(0x24, ARG_REG_H, ARG_NONE, gb_inc_u8, 1, "INC H"),
    GB_OPCODE(0x2C, ARG_REG_L, ARG_NONE, gb_inc_u8, 1, "INC L"),
    GB_OPCODE(0x3C, ARG_REG_A, ARG_NONE, gb_inc_u8, 1, "INC A"),
    GB_OPCODE(0x34, ARG_IND_HL,ARG_NONE, gb_inc_u8, 3, "INC (HL)"),

    GB_OPCODE(0x02, ARG_IND_BC, ARG_REG_A, gb_ld_u8, 2, "LD (BC),A"),
    GB_OPCODE(0x0A, ARG_REG_A, ARG_IND_BC, gb_ld_u8, 2, "LD A,(BC)"),
    GB_OPCODE(0x12, ARG_IND_DE, ARG_REG_A, gb_ld_u8, 2, "LD (DE),A"),
    GB_OPCODE(0x1A, ARG_REG_A, ARG_IND_DE, gb_ld_u8, 2, "LD A,(DE)"),
    GB_OPCODE(0x22, ARG_IND_HLI, ARG_REG_A, gb_ld_u8, 2, "LD (HL+),A"),
    GB_OPCODE(0x2A, ARG_REG_A, ARG_IND_HLI, gb_ld_u8, 2, "LD A,(HL+)"),
    GB_OPCODE(0x32, ARG_IND_HLD, ARG_REG_A, gb_ld_u8, 2, "LD (HL-),A"),
    GB_OPCODE(0x3A, ARG_REG_A, ARG_IND_HLD, gb_ld_u8, 2, "LD A,(HL-)"),
    GB_OPCODE(0x40, ARG_REG_B, ARG_REG_B, gb_ld_u8, 1, "LD B,B"),
    GB_OPCODE(0x41, ARG_REG_B, ARG_REG_C, gb_ld_u8, 1, "LD B,C"),
    GB_OPCODE(0x42, ARG_REG_B, ARG_REG_D, gb_ld_u8, 1, "LD B,D"),
    GB_OPCODE(0x43, ARG_REG_B, ARG_REG_E, gb_ld_u8, 1, "LD B,E"),
    GB_OPCODE(0x44, ARG_REG_B, ARG_REG_H, gb_ld_u8, 1, "LD B,H"),
    GB_OPCODE(0x45, ARG_REG_B, ARG_REG_L, gb_ld_u8, 1, "LD B,L"),
    GB_OPCODE(0x46, ARG_REG_B, ARG_IND_HL, gb_ld_u8, 2, "LD B,(HL)"),
    GB_OPCODE(0x47, ARG_REG_B, ARG_REG_A, gb_ld_u8, 1, "LD B,A"),
    GB_OPCODE(0x48, ARG_REG_C, ARG_REG_B, gb_ld_u8, 1, "LD C,B"),
    GB_OPCODE(0x49, ARG_REG_C, ARG_REG_C, gb_ld_u8, 1, "LD C,C"),
    GB_OPCODE(0x4A, ARG_REG_C, ARG_REG_D, gb_ld_u8, 1, "LD C,D"),
    GB_OPCODE(0x4B, ARG_REG_C, ARG_REG_E, gb_ld_u8, 1, "LD C,E"),
    GB_OPCODE(0x4C, ARG_REG_C, ARG_REG_H, gb_ld_u8, 1, "LD C,H"),
    GB_OPCODE(0x4D, ARG_REG_C, ARG_REG_L, gb_ld_u8, 1, "LD C,L"),
    GB_OPCODE(0x4E, ARG_REG_C, ARG_IND_HL, gb_ld_u8, 2, "LD C,(HL)"),
    GB_OPCODE(0x4F, ARG_REG_C, ARG_REG_A, gb_ld_u8, 1, "LD C,A"),
    GB_OPCODE(0x50, ARG_REG_D, ARG_REG_B, gb_ld_u8, 1, "LD D,B"),
    GB_OPCODE(0x51, ARG_REG_D, ARG_REG_C, gb_ld_u8, 1, "LD D,C"),
    GB_OPCODE(0x52, ARG_REG_D, ARG_REG_D, gb_ld_u8, 1, "LD D,D"),
    GB_OPCODE(0x53, ARG_REG_D, ARG_REG_E, gb_ld_u8, 1, "LD D,E"),
    GB_OPCODE(0x54, ARG_REG_D, ARG_REG_H, gb_ld_u8, 1, "LD D,H"),
    GB_OPCODE(0x55, ARG_REG_D, ARG_REG_L, gb_ld_u8, 1, "LD D,L"),
    GB_OPCODE(0x56, ARG_REG_D, ARG_IND_HL, gb_ld_u8, 2, "LD D,(HL)"),
    GB_OPCODE(0x57, ARG_REG_D, ARG_REG_A, gb_ld_u8, 1, "LD D,A"),
    GB_OPCODE(0x58, ARG_REG_E, ARG_REG_B, gb_ld_u8, 1, "LD E,B"),
    GB_OPCODE(0x59, ARG_REG_E, ARG_REG_C, gb_ld_u8, 1, "LD E,C"),
    GB_OPCODE(0x5A, ARG_REG_E, ARG_REG_D, gb_ld_u8, 1, "LD E,D"),
    GB_OPCODE(0x5B, ARG_REG_E, ARG_REG_E, gb_ld_u8, 1, "LD E,E"),
    GB_OPCODE(0x5C, ARG_REG_E, ARG_REG_H, gb_ld_u8, 1, "LD E,H"),
    GB_OPCODE(0x5D, ARG_REG_E, ARG_REG_L, gb_ld_u8, 1, "LD E,L"),
    GB_OPCODE(0x5E, ARG_REG_E, ARG_IND_HL, gb_ld_u8, 2, "LD E,(HL)"),
    GB_OPCODE(0x5F, ARG_REG_E, ARG_REG_A, gb_ld_u8, 1, "LD E,A"),
    GB_OPCODE(0x60, ARG_REG_H, ARG_REG_B, gb_ld_u8, 1, "LD H,B"),
    GB_OPCODE(0x61, ARG_REG_H, ARG_REG_C, gb_ld_u8, 1, "LD H,C"),
    GB_OPCODE(0x62, ARG_REG_H, ARG_REG_D, gb_ld_u8, 1, "LD H,D"),
    GB_OPCODE(0x63, ARG_REG_H, ARG_REG_E, gb_ld_u8, 1, "LD H,E"),
    GB_OPCODE(0x64, ARG_REG_H, ARG_REG_H, gb_ld_u8, 1, "LD H,H"), 
    GB_OPCODE(0x65, ARG_REG_H, ARG_REG_L, gb_ld_u8, 1, "LD H,L"), 
    GB_OPCODE(0x66, ARG_REG_H, ARG_IND_HL, gb_ld_u8, 2, "LD H,(HL)"), 
    GB_OPCODE(0x67, ARG_REG_H, ARG_REG_A, gb_ld_u8, 1, "LD H,A"), 
    GB_OPCODE(0x68, ARG_REG_L, ARG_REG_B, gb_ld_u8, 1, "LD L,B"), 
    GB_OPCODE(0x69, ARG_REG_L, ARG_REG_C, gb_ld_u8, 1, "LD L,C"), 
    GB_OPCODE(0x6A, ARG_REG_L, ARG_REG_D, gb_ld_u8, 1, "LD L,D"), 
    GB_OPCODE(0x6B, ARG_REG_L, ARG_REG_E, gb_ld_u8, 1, "LD L,E"), 
    GB_OPCODE(0x6C, ARG_REG_L, ARG_REG_H, gb_ld_u8, 1, "LD L,H"), 
    GB_OPCODE(0x6D, ARG_REG_L, ARG_REG_L, gb_ld_u8, 1, "LD L,L"), 
    GB_OPCODE(0x6E, ARG_REG_L, ARG_IND_HL, gb_ld_u8, 2, "LD L,(HL)"), 
    GB_OPCODE(0x6F, ARG_REG_L, ARG_REG_A, gb_ld_u8, 1, "LD L,A"), 
    GB_OPCODE(0x70, ARG_IND_HL, ARG_REG_B, gb_ld_u8, 2, "LD (HL),B"), 
    GB_OPCODE(0x71, ARG_IND_HL, ARG_REG_C, gb_ld_u8, 2, "LD (HL),C"), 
    GB_OPCODE(0x72, ARG_IND_HL, ARG_REG_D, gb_ld_u8, 2, "LD (HL),D"), 
    GB_OPCODE(0x73, ARG_IND_HL, ARG_REG_E, gb_ld_u8, 2, "LD (HL),E"), 
    GB_OPCODE(0x74, ARG_IND_HL, ARG_REG_H, gb_ld_u8, 2, "LD (HL),H"), 
    GB_OPCODE(0x75, ARG_IND_HL, ARG_REG_L, gb_ld_u8, 2, "LD (HL),L"), 
    GB_OPCODE(0x77, ARG_IND_HL, ARG_REG_A, gb_ld_u8, 2, "LD (HL),A"), 
    GB_OPCODE(0x78, ARG_REG_A, ARG_REG_B, gb_ld_u8, 1, "LD A,B"), 
    GB_OPCODE(0x79, ARG_REG_A, ARG_REG_C, gb_ld_u8, 1, "LD A,C"), 
    GB_OPCODE(0x7A, ARG_REG_A, ARG_REG_D, gb_ld_u8, 1, "LD A,D"), 
    GB_OPCODE(0x7B, ARG_REG_A, ARG_REG_E, gb_ld_u8, 1, "LD A,E"), 
    GB_OPCODE(0x7C, ARG_REG_A, ARG_REG_H, gb_ld_u8, 1, "LD A,H"), 
    GB_OPCODE(0x7D, ARG_REG_A, ARG_REG_L, gb_ld_u8, 1, "LD A,L"), 
    GB_OPCODE(0x7E, ARG_REG_A, ARG_IND_HL, gb_ld_u8, 2, "LD A,(HL)"), 
    GB_OPCODE(0x7F, ARG_REG_A, ARG_REG_A, gb_ld_u8, 1, "LD A,A"), 
    GB_OPCODE(0xE0, ARG_IND_IMM8, ARG_REG_A, gb_ld_u8, 3, "LD ($%02X),A"), 
    GB_OPCODE(0xF0, ARG_REG_A, ARG_IND_IMM8, gb_ld_u8, 3, "LD A,($%02X)"),
    GB_OPCODE(0xEA, ARG_IND_IMM16, ARG_REG_A, gb_ld_u8, 4, "LD ($%04X),A"),
    GB_OPCODE(0xFA, ARG_REG_A, ARG_IND_IMM16, gb_ld_u8, 4, "LD A,($%04X)"),
    GB_OPCODE(0x06, ARG_REG_B, ARG_IMM8, gb_ld_u8, 2, "LD B,$%02X"),

    GB_OPCODE(0x0E, ARG_REG_C, ARG_IMM8, gb_ld_u8, 2, "LD C,$%02X"),
    GB_OPCODE(0x16, ARG_REG_D, ARG_IMM8, gb_ld_u8, 2, "LD D,$%02X"),
    GB_OPCODE(0x1E, ARG_REG_E, ARG_IMM8, gb_ld_u8, 2, "LD E,$%02X"),
    GB_OPCODE(0x26, ARG_REG_H, ARG_IMM8, gb_ld_u8, 2, "LD H,$%02X"),
    GB_OPCODE(0x2E, ARG_REG_L, ARG_IMM8, gb_ld_u8, 2, "LD L,$%02X"),
    GB_OPCODE(0x36, ARG_IND_HL, ARG_IMM8, gb_ld_u8, 3, "LD (HL),$%02X"),
    GB_OPCODE(0x3E, ARG_REG_A, ARG_IMM8, gb_ld_u8, 2, "LD A,$%02X"),

    GB_OPCODE(0x01, ARG_REG_BC, ARG_IMM16, gb_ld_u16, 3, "LD BC,$%04X"),
    GB_OPCODE(0x11, ARG_REG_DE, ARG_IMM16, gb_ld_u16, 3, "LD DE,$%04X"),
    GB_OPCODE(0x21, ARG_REG_HL, ARG_IMM16, gb_ld_u16, 3, "LD HL,$%04X"),
    GB_OPCODE(0x31, ARG_REG_SP, ARG_IMM16, gb_ld_u16, 3, "LD SP,$%04X"),
    GB_OPCODE(0x08, ARG_IND_IMM16, ARG_REG_SP, gb_ld_u16, 5, "LD ($%04X),SP"),
    GB_OPCODE(0xF9, ARG_REG_SP, ARG_REG_HL, gb_ld_u16, 2, "LD SP,HL"),

    GB_OPCODE(0x80, ARG_REG_A, ARG_REG_B, gb_add_u8, 1, "ADD A,B"),
    GB_OPCODE(0x81, ARG_REG_A, ARG_REG_C, gb_add_u8, 1, "ADD A,C"),
    GB_OPCODE(0x82, ARG_REG_A, ARG_REG_D, gb_add_u8, 1, "ADD A,D"),
    GB_OPCODE(0x83, ARG_REG_A, ARG_REG_E, gb_add_u8, 1, "ADD A,E"),
    GB_OPCODE(0x84, ARG_REG_A, ARG_REG_H, gb_add_u8, 1, "ADD A,H"),
    GB_OPCODE(0x85, ARG_REG_A, ARG_REG_L, gb_add_u8, 1, "ADD A,L"),
    GB_OPCODE(0x86, ARG_REG_A, ARG_IND_HL, gb_add_u8, 2, "ADD A,(HL)"),
    GB_OPCODE(0x87, ARG_REG_A, ARG_REG_A, gb_add_u8, 1, "ADD A,A"),
    GB_OPCODE(0xC6, ARG_REG_A, ARG_IMM8, gb_add_u8, 2, "ADD A,$%02X"),

    GB_OPCODE(0x88, ARG_REG_A, ARG_REG_B, gb_adc_u8, 1, "ADC A,B"),
    GB_OPCODE(0x89, ARG_REG_A, ARG_REG_C, gb_adc_u8, 1, "ADC A,C"),
    GB_OPCODE(0x8A, ARG_REG_A, ARG_REG_D, gb_adc_u8, 1, "ADC A,D"),
    GB_OPCODE(0x8B, ARG_REG_A, ARG_REG_E, gb_adc_u8, 1, "ADC A,E"),
    GB_OPCODE(0x8C, ARG_REG_A, ARG_REG_H, gb_adc_u8, 1, "ADC A,H"),
    GB_OPCODE(0x8D, ARG_REG_A, ARG_REG_L, gb_adc_u8, 1, "ADC A,L"),
    GB_OPCODE(0x8E, ARG_REG_A, ARG_IND_HL, gb_adc_u8, 2, "ADC A,(HL)"),
    GB_OPCODE(0x8F, ARG_REG_A, ARG_REG_A, gb_adc_u8, 1, "ADC A,A"),
    GB_OPCODE(0xCE, ARG_REG_A, ARG_IMM8, gb_adc_u8, 2, "ADC A,$%02X"),

    GB_OPCODE(0x90, ARG_REG_A, ARG_REG_B, gb_sub_u8, 1, "SUB A,B"),
    GB_OPCODE(0x91, ARG_REG_A, ARG_REG_C, gb_sub_u8, 1, "SUB A,C"),
    GB_OPCODE(0x92, ARG_REG_A, ARG_REG_D, gb_sub_u8, 1, "SUB A,D"),
    GB_OPCODE(0x93, ARG_REG_A, ARG_REG_E, gb_sub_u8, 1, "SUB A,E"),
    GB_OPCODE(0x94, ARG_REG_A, ARG_REG_H, gb_sub_u8, 1, "SUB A,H"),
    GB_OPCODE(0x95, ARG_REG_A, ARG_REG_L, gb_sub_u8, 1, "SUB A,L"),
    GB_OPCODE(0x96, ARG_REG_A, ARG_IND_HL, gb_sub_u8, 2, "SUB A,(HL)"),
    GB_OPCODE(0x97, ARG_REG_A, ARG_REG_A, gb_sub_u8, 1, "SUB A,A"),
    GB_OPCODE(0xD6, ARG_REG_A, ARG_IMM8, gb_sub_u8, 2, "SUB A,$%02X"),

    GB_OPCODE(0x98, ARG_REG_A, ARG_REG_B, gb_sbc_u8, 1, "SBC A,B"),
    GB_OPCODE(0x99, ARG_REG_A, ARG_REG_C, gb_sbc_u8, 1, "SBC A,C"),
    GB_OPCODE(0x9A, ARG_REG_A, ARG_REG_D, gb_sbc_u8, 1, "SBC A,D"),
    GB_OPCODE(0x9B, ARG_REG_A, ARG_REG_E, gb_sbc_u8, 1, "SBC A,E"),
    GB_OPCODE(0x9C, ARG_REG_A, ARG_REG_H, gb_sbc_u8, 1, "SBC A,H"),
    GB_OPCODE(0x9D, ARG_REG_A, ARG_REG_L, gb_sbc_u8, 1, "SBC A,L"),
    GB_OPCODE(0x9E, ARG_REG_A, ARG_IND_HL, gb_sbc_u8, 2, "SBC A,(HL)"),
    GB_OPCODE(0x9F, ARG_REG_A, ARG_REG_A, gb_sbc_u8, 1, "SBC A,A"),
    GB_OPCODE(0xDE, ARG_REG_A, ARG_IMM8, gb_sbc_u8, 2, "SBC A,$%02X"),

    GB_OPCODE(0xC3, ARG_IMM16, ARG_NONE, gb_jp_a16, 4, "JP $%04X"),

    GB_OPCODE(0xA8, ARG_REG_B, ARG_NONE, gb_xor_u8, 1, "XOR B"),
    GB_OPCODE(0xA9, ARG_REG_C, ARG_NONE, gb_xor_u8, 1, "XOR C"),
    GB_OPCODE(0xAA, ARG_REG_D, ARG_NONE, gb_xor_u8, 1, "XOR D"),
    GB_OPCODE(0xAB, ARG_REG_E, ARG_NONE, gb_xor_u8, 1, "XOR E"),
    GB_OPCODE(0xAC, ARG_REG_H, ARG_NONE, gb_xor_u8, 1, "XOR H"),
    GB_OPCODE(0xAD, ARG_REG_L, ARG_NONE, gb_xor_u8, 1, "XOR L"),
    GB_OPCODE(0xAE, ARG_IND_HL, ARG_NONE, gb_xor_u8, 2, "XOR (HL)"),
    GB_OPCODE(0xAF, ARG_REG_A, ARG_NONE, gb_xor_u8, 1, "XOR A"),
};

const gb_opcode_t gb_cb_opcodes[256] = {
    GB_OPCODE(0x00, ARG_NONE, ARG_NONE, gb_nop, 1, "NOP"), // NOP
};

