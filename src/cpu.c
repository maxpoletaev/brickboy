#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "shared.h"
#include "cpu.h"
#include "bus.h"

static const uint16_t gb_reset_addr[8] = {
    0x00, 0x08, 0x10, 0x18,
    0x20, 0x28, 0x30, 0x38,
};

const char *gb_operand_names[] = {
    [ARG_NONE] = "NONE",

    [ARG_IMM8] = "IMM8",
    [ARG_IMM16] = "IMM16",
    [ARG_REG_A] = "REG_A",
    [ARG_REG_B] = "REG_B",
    [ARG_REG_C] = "REG_C",
    [ARG_REG_D] = "REG_D",
    [ARG_REG_E] = "REG_E",
    [ARG_REG_H] = "REG_H",
    [ARG_REG_L] = "REG_L",
    [ARG_REG_AF] = "REG_AF",
    [ARG_REG_BC] = "REG_BC",
    [ARG_REG_DE] = "REG_DE",
    [ARG_REG_HL] = "REG_HL",
    [ARG_REG_SP] = "REG_SP",

    [ARG_IND_C] = "IND_C",
    [ARG_IND_BC] = "IND_BC",
    [ARG_IND_DE] = "IND_DE",
    [ARG_IND_HL] = "IND_HL",
    [ARG_IND_HLI] = "IND_HLI",
    [ARG_IND_HLD] = "IND_HLD",
    [ARG_IND_IMM8] = "IND_IMM8",
    [ARG_IND_IMM16] = "IND_IMM16",
    [ARG_FLAG_ZERO] = "FLAG_ZERO",
    [ARG_FLAG_CARRY] = "FLAG_CARRY",

    [ARG_BIT_0] = "BIT_0",
    [ARG_BIT_1] = "BIT_1",
    [ARG_BIT_2] = "BIT_2",
    [ARG_BIT_3] = "BIT_3",
    [ARG_BIT_4] = "BIT_4",
    [ARG_BIT_5] = "BIT_5",
    [ARG_BIT_6] = "BIT_6",
    [ARG_BIT_7] = "BIT_7",

    [ARG_RST_0] = "RST_0",
    [ARG_RST_1] = "RST_1",
    [ARG_RST_2] = "RST_2",
    [ARG_RST_3] = "RST_3",
    [ARG_RST_4] = "RST_4",
    [ARG_RST_5] = "RST_5",
    [ARG_RST_6] = "RST_6",
    [ARG_RST_7] = "RST_7",
};

void
gb_cpu_reset(gb_cpu_t *cpu)
{
    cpu->af = 0x01B0;
    cpu->bc = 0x0013;
    cpu->de = 0x00D8;
    cpu->hl = 0x014D;
    cpu->sp = 0xFFFE;
    cpu->pc = 0x0100; // skip the boot rom for now
    cpu->ime_delay = -1;
    cpu->ime = 1;
    cpu->stall = 0;
    cpu->cycle = 0;
}

static uint16_t
gb_cpu_get(gb_cpu_t *cpu, gb_bus_t *bus, gb_operand_t src)
{
    uint16_t value = 0;
    uint16_t addr = 0;

    switch (src) {
    case ARG_NONE:
        GB_BOUNDS_CHECK(gb_operand_names, src);
        GB_PANIC("invalid source operand: %s", gb_operand_names[src]);
    case ARG_BIT_0:
    case ARG_BIT_1:
    case ARG_BIT_2:
    case ARG_BIT_3:
    case ARG_BIT_4:
    case ARG_BIT_5:
    case ARG_BIT_6:
    case ARG_BIT_7:
        value = (uint16_t) (src - ARG_BIT_0); // just the bit number
        break;
    case ARG_RST_0:
    case ARG_RST_1:
    case ARG_RST_2:
    case ARG_RST_3:
    case ARG_RST_4:
    case ARG_RST_5:
    case ARG_RST_6:
    case ARG_RST_7:
        value = gb_reset_addr[src - ARG_RST_0];
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
    case ARG_IND_C:
        addr = 0xFF00 + cpu->c;
        value = gb_bus_read(bus, addr);
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
        addr = 0xFF00 + gb_bus_read(bus, cpu->pc++);
        value = gb_bus_read(bus, addr);
        break;
    case ARG_IND_IMM16:
        addr = gb_bus_read16(bus, cpu->pc);
        value = gb_bus_read(bus, addr);
        cpu->pc += 2;
        break;
    case ARG_FLAG_CARRY:
        value = (uint16_t) cpu->flags.carry;
        break;
    case ARG_FLAG_ZERO:
        value = (uint16_t) cpu->flags.zero;
        break;
    }

    return value;
}

static void
gb_cpu_set(gb_cpu_t *cpu, gb_bus_t *bus, gb_operand_t target, uint16_t value16)
{
    uint8_t value = (uint8_t) value16;
    uint16_t addr = 0;

    switch (target) {
    case ARG_NONE:
    case ARG_IMM8:
    case ARG_IMM16:
    case ARG_BIT_0:
    case ARG_BIT_1:
    case ARG_BIT_2:
    case ARG_BIT_3:
    case ARG_BIT_4:
    case ARG_BIT_5:
    case ARG_BIT_6:
    case ARG_BIT_7:
    case ARG_RST_0:
    case ARG_RST_1:
    case ARG_RST_2:
    case ARG_RST_3:
    case ARG_RST_4:
    case ARG_RST_5:
    case ARG_RST_6:
    case ARG_RST_7:
    case ARG_FLAG_ZERO:
    case ARG_FLAG_CARRY:
        GB_BOUNDS_CHECK(gb_operand_names, target);
        GB_PANIC("invalid target operand: %s", gb_operand_names[target]);
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
        break;
    case ARG_REG_SP:
        cpu->sp = value16;
        break;
    case ARG_IND_C:
        addr = 0xFF00 + cpu->c;
        gb_bus_write(bus, addr, value);
        break;
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
    case ARG_IND_IMM8:
        addr = 0xFF00 + gb_bus_read(bus, cpu->pc++);
        gb_bus_write(bus, addr, value);
        break;
    case ARG_IND_IMM16:
        addr = gb_bus_read16(bus, cpu->pc);
        gb_bus_write(bus, addr, value);
        cpu->pc += 2;
        break;
    }
}

static uint8_t
gb_cpu_getbit(gb_cpu_t *cpu, gb_bus_t *bus, gb_operand_t arg, uint8_t bit)
{
    uint8_t value = (uint8_t) gb_cpu_get(cpu, bus, arg);
    return (value >> bit) & 1;
}

static void
gb_cpu_setbit(gb_cpu_t *cpu, gb_bus_t *bus, gb_operand_t arg, uint8_t bit, uint8_t value)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, arg);

    assert(bit < 8);
    v &= ~(1 << bit); // clear bit first
    v |= (value & 1) << bit; // OR in the new value

    gb_cpu_set(cpu, bus, arg, v);
}

static void
gb_cpu_push(gb_cpu_t *cpu, gb_bus_t *bus, uint16_t value)
{
    cpu->sp -= 2;
    gb_bus_write16(bus, cpu->sp, value);
}

static uint16_t
gb_cpu_pop(gb_cpu_t *cpu, gb_bus_t *bus)
{
    uint16_t value = gb_bus_read16(bus, cpu->sp);
    cpu->sp += 2;
    return value;
}

const gb_instr_t *
gb_cpu_decode(gb_bus_t *bus, uint16_t pc)
{
    uint8_t opcode = gb_bus_read(bus, pc);
    const gb_instr_t *op = &gb_opcodes[opcode];

    // Prefixed instructions
    if (opcode == 0xCB) {
        opcode = gb_bus_read(bus, pc+1);
        op = &gb_cb_opcodes[opcode];
    }

    return op;
}

void
gb_cpu_step(gb_cpu_t *cpu, gb_bus_t *bus)
{
    cpu->cycle++;

    if (cpu->stall > 0) {
        cpu->stall--;
        return;
    }

    uint8_t opcode = gb_bus_read(bus, cpu->pc++);
    const gb_instr_t *op = &gb_opcodes[opcode];

    // Prefixed instructions
    if (opcode == 0xCB) {
        opcode = gb_bus_read(bus, cpu->pc++);
        op = &gb_cb_opcodes[opcode];
    }

    if (op->handler == NULL)
        GB_PANIC("invalid opcode: 0x%02X", opcode);

    cpu->stall = op->cycles - 1;
    op->handler(cpu, bus, op);

    // DI/EI take effect after the next instruction
    if (cpu->ime_delay >= 0) {
        cpu->ime = (uint8_t) cpu->ime_delay;
        cpu->ime_delay = -1;
    }
}

/* ----------------------------------------------------------------------------
 * Instruction Handlers
 * -------------------------------------------------------------------------- */

/* NOP
 * Flags: - - - -
 * No operation. */
static void
gb_nop(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    GB_UNUSED(cpu);
    GB_UNUSED(bus);
    GB_UNUSED(op);
}

/* DAA
 * Flags: Z - 0 C
 * Decimal adjust register A. */
static void
gb_daa(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    GB_UNUSED(bus);
    GB_UNUSED(op);

    uint8_t a = cpu->a;
    uint8_t correction = cpu->flags.carry ? 0x60 : 0x00;

    if (cpu->flags.half_carry || (!cpu->flags.negative && (a&0xF) > 9))
        correction |= 0x06;

    if (cpu->flags.carry || (!cpu->flags.negative && a > 0x99))
        correction |= 0x60;

    if (cpu->flags.negative) {
        a -= correction;
    } else {
        a += correction;
    }

    cpu->flags.zero = a == 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = correction >= 0x60;

    cpu->a = a;
}

/* CPL
 * Flags: - 1 1 -
 * Complement A register. */
static void
gb_cpl(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    GB_UNUSED(bus);
    GB_UNUSED(op);

    cpu->a = ~cpu->a;
    cpu->flags.negative = 1;
    cpu->flags.half_carry = 1;
}

/* SCF
 * Flags: - 0 0 1
 * Set carry flag. */
static void
gb_scf(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    GB_UNUSED(bus);
    GB_UNUSED(op);

    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = 1;
}

/* CCF
 * Flags: - 0 0 C
 * Complement carry flag. */
static void
gb_ccf(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    GB_UNUSED(bus);
    GB_UNUSED(op);

    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = !cpu->flags.carry;
}

/* LD r8,r8
 * Flags: - - - -
 * Load 8-bit register or memory location into another 8-bit register or memory location. */
static void
gb_ld8(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg2);
    gb_cpu_set(cpu, bus, op->arg1, v);
}

/* LD r16,r16
 * Flags: - - - -
 * Load 16-bit data into 16-bit register. */
static void
gb_ld16(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint16_t v = gb_cpu_get(cpu, bus, op->arg2);
    gb_cpu_set(cpu, bus, op->arg1, v);
}

/* INC r8
 * Flags: Z 0 H -
 * Increment 8-bit register or memory location. */
static void
gb_inc8(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);

    uint8_t half_sum = (v&0xF) + 1;
    uint8_t r = v + 1;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = half_sum > 0xF;

    gb_cpu_set(cpu, bus, op->arg1, r);
}

/* INC r16
 * Flags: - - - -
 * Increment 16-bit register. */
static void
gb_inc16(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint16_t v = gb_cpu_get(cpu, bus, op->arg1);
    uint16_t r = v + 1;

    gb_cpu_set(cpu, bus, op->arg1, r);
}

/* DEC r8
 * Flags: Z 1 H -
 * Decrement 8-bit register or memory location. */
static void
gb_dec8(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t r = v - 1;

    uint8_t half_sum = (v&0xF) - 1;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 1;
    cpu->flags.half_carry = half_sum > 0xF;

    gb_cpu_set(cpu, bus, op->arg1, r);
}

/* DEC r16
 * Flags: - - - -
 * Decrement 16-bit register. */
static void
gb_dec16(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint16_t v = gb_cpu_get(cpu, bus, op->arg1);
    uint16_t r = v - 1;

    gb_cpu_set(cpu, bus, op->arg1, r);
}

/* ADD A,r8
 * Flags: Z 0 H C
 * Add 8-bit register or memory location to A. */
static void
gb_add_a(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t a = cpu->a;

    uint16_t sum = (uint16_t) a + (uint16_t) v;
    uint8_t half_sum = (a&0xF) + (v&0xF);
    uint8_t r = (uint8_t) sum;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = half_sum > 0xF;
    cpu->flags.carry = sum > 0xFF;

    cpu->a = r;
}

/* Add HL,r16
 * Flags: - 0 H C
 * Add 16-bit register to HL. */
static void
gb_add_hl(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint16_t v = gb_cpu_get(cpu, bus, op->arg1);
    uint16_t hl = cpu->hl;

    uint16_t half_sum = (hl&0x0FFF) + (v&0x0FFF);
    uint32_t sum = (uint32_t) hl + (uint32_t) v;
    uint16_t r = (uint16_t) sum;

    cpu->flags.negative = 0;
    cpu->flags.half_carry = half_sum > 0x0FFF;
    cpu->flags.carry = sum > 0xFFFF;

    cpu->hl = r;
}

/* ADD SP,s8
 * Flags: 0 0 H C
 * Add 8-bit signed immediate to SP. */
static void
gb_add_sp(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    int8_t v = (int8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint16_t sp = cpu->sp;

    int32_t sum = (int32_t) sp + (int32_t) v;
    uint8_t half_sum = (sp&0xF) + (v&0xF);
    uint16_t r = (uint16_t) sum;

    cpu->flags.zero = 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = half_sum > 0xF;
    cpu->flags.carry = sum > 0xFFFF;

    cpu->sp = r;
}

/* ADC A,r8
 * Flags: Z 0 H C
 * Add 8-bit register or memory location and carry flag to A. */
static void
gb_adc8(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t c = cpu->flags.carry;
    uint8_t a = cpu->a;

    uint16_t sum = (uint16_t) a + (uint16_t) v + (uint16_t) c;
    uint8_t half_sum = (a&0xF) + (v&0xF) + c;
    uint8_t r = (uint8_t) sum;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = half_sum > 0xF;
    cpu->flags.carry = sum > 0xFF;

    cpu->a = r;
}

/* SUB A,r8
 * Flags: Z 1 H C
 * Subtract 8-bit register or memory location from A. */
static void
gb_sub8(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t a = cpu->a;

    uint16_t sum = (uint16_t) a - (uint16_t) v;
    uint8_t half_sum = (a&0xF) - (v&0xF);
    uint8_t r = (uint8_t) sum;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 1;
    cpu->flags.half_carry = half_sum > 0xF;
    cpu->flags.carry = sum > 0xFF;

    cpu->a = r;
}

/* SBC A,r8
 * Flags: Z 1 H C
 * Subtract 8-bit register or memory location and carry flag from A. */
static void
gb_sbc8(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t c = cpu->flags.carry;
    uint8_t a = cpu->a;

    uint16_t sum = (uint16_t) a - (uint16_t) v - (uint16_t) c;
    uint8_t half_sum = (a&0xF) - (v&0xF) - c;
    uint8_t r = (uint8_t) sum;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 1;
    cpu->flags.half_carry = half_sum > 0xF;
    cpu->flags.carry = sum > 0xFF;

    cpu->a = r;
}

/* AND r8
 * Flags: Z 0 1 0
 * AND 8-bit register or memory location with A. */
static void
gb_and8(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t a = cpu->a;
    uint8_t r = a & v;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 1;
    cpu->flags.carry = 0;

    cpu->a = r;
}

/* JP a16
 * Flags: - - - -
 * Jump to 16-bit address provided by immediate operand or register. */
static void
gb_jp16(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint16_t addr = gb_cpu_get(cpu, bus, op->arg1);
    cpu->pc = addr;
}

static void
gb_jp16_if(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    bool flag = (bool) gb_cpu_get(cpu, bus, op->arg1);
    uint16_t addr = gb_cpu_get(cpu, bus, op->arg2);

    if (flag) {
        cpu->pc = addr;
        cpu->stall++;
    }
}

static void
gb_jp16_ifn(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    bool flag = (bool) gb_cpu_get(cpu, bus, op->arg1);
    uint16_t addr = gb_cpu_get(cpu, bus, op->arg2);

    if (!flag) {
        cpu->pc = addr;
        cpu->stall++;
    }
}

/* JR s8
 * Flags: - - - -
 * Jump to 8-bit signed offset. */
static void
gb_jr8(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    int8_t offset = (int8_t) gb_cpu_get(cpu, bus, op->arg1);
    cpu->pc += offset;
}

/* JR F,s8
 * Flags: - - - -
 * Jump to 8-bit signed offset if F flag is set. */
static void
gb_jr8_if(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    bool flag = (bool) gb_cpu_get(cpu, bus, op->arg1);
    int8_t offset = (int8_t) gb_cpu_get(cpu, bus, op->arg2);

    if (flag) {
        cpu->pc += offset;
        cpu->stall += 1;
    }
}

/* JR !F,s8
 * Flags: - - - -
 * Jump to 8-bit signed offset if F flag is not set. */
static void
gb_jr8_ifn(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    bool flag = (bool) gb_cpu_get(cpu, bus, op->arg1);
    int8_t offset = (int8_t) gb_cpu_get(cpu, bus, op->arg2);

    if (!flag) {
        cpu->pc += offset;
        cpu->stall += 1;
    }
}

/* XOR r8
 * Flags: Z 0 0 0
 * XOR 8-bit register or memory location with A. */
static void
gb_xor8(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t a = cpu->a;
    uint8_t r = a ^ v;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = 0;

    cpu->a = r;
}

/* OR r8
 * Flags: Z 0 0 0
 * OR 8-bit register or memory location with A. */
static void
gb_or8(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t a = cpu->a;
    uint8_t r = a | v;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = 0;

    cpu->a = r;
}

/* CP r8
 * Flags: Z 1 H C
 * Compare 8-bit register or memory location with A. */
static void
gb_cp8(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t a = cpu->a;

    uint16_t sum = (uint16_t) a - (uint16_t) v;
    uint8_t half_sum = (a&0xF) - (v&0xF);
    uint8_t r = (uint8_t) sum;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 1;
    cpu->flags.half_carry = half_sum > 0xF;
    cpu->flags.carry = sum > 0xFF;
}

/* PUSH r16
 * Flags: - - - -
 * Push 16-bit register onto stack. */
static void
gb_push16(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint16_t v = gb_cpu_get(cpu, bus, op->arg1);
    gb_cpu_push(cpu, bus, v);
}

/* POP r16
 * Flags: - - - -
 * Pop 16-bit register off stack. */
static void
gb_pop16(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint16_t v = gb_cpu_pop(cpu, bus);

    if (op->arg1 == ARG_REG_AF)
        v &= 0xFFF0; // lower 4 bits of F are always zero

    gb_cpu_set(cpu, bus, op->arg1, v);
}

/* CALL a16
 * Flags: - - - -
 * Call 16-bit address provided by immediate operand or register. */
static void
gb_call(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint16_t addr = gb_cpu_get(cpu, bus, op->arg1);
    gb_cpu_push(cpu, bus, cpu->pc);
    cpu->pc = addr;
}

/* CALL F,a16
 * Flags: - - - -
 * Call 16-bit address provided by immediate operand or register if F flag is set. */
static void
gb_call_if(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    bool flag = (bool) gb_cpu_get(cpu, bus, op->arg1);
    uint16_t addr = gb_cpu_get(cpu, bus, op->arg2);

    if (flag) {
        gb_cpu_push(cpu, bus, cpu->pc);
        cpu->stall += 3;
        cpu->pc = addr;
    }
}

/* CALL !F,a16
 * Flags: - - - -
 * Call 16-bit address provided by immediate operand or register if F flag is not set. */
static void
gb_call_ifn(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    bool flag = (bool) gb_cpu_get(cpu, bus, op->arg1);
    uint16_t addr = gb_cpu_get(cpu, bus, op->arg2);

    if (!flag) {
        gb_cpu_push(cpu, bus, cpu->pc);
        cpu->stall += 3;
        cpu->pc = addr;
    }
}

/* RET
 * Flags: - - - -
 * Return from subroutine. */
static void
gb_ret(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    GB_UNUSED(bus);
    GB_UNUSED(op);

    cpu->pc = gb_cpu_pop(cpu, bus);
}

/* RET F
 * Flags: - - - -
 * Return from subroutine if F flag is set. */
static void
gb_ret_if(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    bool flag = (bool) gb_cpu_get(cpu, bus, op->arg1);

    if (flag) {
        cpu->pc = gb_cpu_pop(cpu, bus);
        cpu->stall += 3;
    }
}

/* RET !F
 * Flags: - - - -
 * Return from subroutine if F flag is not set. */
static void
gb_ret_ifn(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    bool flag = (bool) gb_cpu_get(cpu, bus, op->arg1);

    if (!flag) {
        cpu->pc = gb_cpu_pop(cpu, bus);
        cpu->stall += 3;
    }
}

/* RETI
 * Flags: - - - -
 * Return from subroutine and enable interrupts. */
static void
gb_reti(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    GB_UNUSED(bus);
    GB_UNUSED(op);

    cpu->pc = gb_cpu_pop(cpu, bus);
    cpu->ime = 1; // not delayed unlike EI
}

/* RST n
 * Flags: - - - -
 * Push present address onto stack and jump to address $0000 + n,
 * where n is one of $00, $08, $10, $18, $20, $28, $30, $38. */
static void
gb_rst(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint16_t addr = gb_cpu_get(cpu, bus, op->arg1);
    gb_cpu_push(cpu, bus, cpu->pc);
    cpu->pc = addr;
}

/* RLCA
 * RLC r8
 * Flags: Z 0 0 C
 * Rotate register left. */
static void
gb_rlc(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t r = (uint8_t) (v << 1) | (v >> 7);

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = (v >> 7) & 1;

    gb_cpu_set(cpu, bus, op->arg1, r);
}

/* RLA
 * Flags: 0 0 0 C
 * Rotate register A left through carry flag. */
static void
gb_rla(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t r = (uint8_t) ((v << 1) | cpu->flags.carry);

    cpu->flags.zero = 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = (v >> 7) & 1;

    gb_cpu_set(cpu, bus, op->arg1, r);
}

/* RL r8
 * Flags: Z 0 0 C
 * Rotate register left through carry flag. */
static void
gb_rl(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t r = (uint8_t) ((v << 1) | cpu->flags.carry);

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = (v >> 7) & 1;

    gb_cpu_set(cpu, bus, op->arg1, r);
}

/* RRCA
 * Flags: 0 0 0 C
 * Rotate register right. */
static void
gb_rrca(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t r = (uint8_t) ((v >> 1) | (v << 7));

    cpu->flags.zero = 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = v & 1;

    gb_cpu_set(cpu, bus, op->arg1, r);
}

/* RRC r8
 * Flags: Z 0 0 C
 * Rotate register right. */
static void
gb_rrc(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t r = (uint8_t) ((v >> 1) | (v << 7));

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = v & 1;

    gb_cpu_set(cpu, bus, op->arg1, r);
}

/* RRA
 * Flags: 0 0 0 C
 * Rotate register A right through carry flag. */
static void
gb_rra(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t r = (uint8_t) ((v >> 1) | (cpu->flags.carry << 7));

    cpu->flags.zero = 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = v & 1;

    gb_cpu_set(cpu, bus, op->arg1, r);
}


/* RR r8
 * Flags: Z 0 0 C
 * Rotate register right through carry flag. */
static void
gb_rr(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t r = (uint8_t) ((v >> 1) | (cpu->flags.carry << 7));

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = v & 1;

    gb_cpu_set(cpu, bus, op->arg1, r);
}

/* SLA n
 * Flags: Z 0 0 C
 * Shift register left into carry. */
static void
gb_sla(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t r = (uint8_t) (v << 1);

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = (v >> 7) & 1;

    gb_cpu_set(cpu, bus, op->arg1, r);
}

/* SRA n
 * Flags: Z 0 0 C
 * Shift register right into carry. MSB is unchanged. */
static void
gb_sra(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t r = (v >> 1) | (v & 0x80);

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = v & 1;

    gb_cpu_set(cpu, bus, op->arg1, r);
}

/* SRL n
 * Flags: Z 0 0 C
 * Shift register right into carry. MSB is set to 0. C flag is old LSB. */
static void
gb_srl(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t r = v >> 1;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = v & 1;

    gb_cpu_set(cpu, bus, op->arg1, r);
}

/* DI
 * Flags: - - - -
 * Disable interrupts. */
static void
gb_di(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    GB_UNUSED(bus);
    GB_UNUSED(op);

    cpu->ime_delay = 0;
}

/* EI
 * Flags: - - - -
 * Enable interrupts. */
static void
gb_ei(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    GB_UNUSED(bus);
    GB_UNUSED(op);

    cpu->ime_delay = 1;
}

/* SWAP n
 * Flags: Z 0 0 0
 * Swap upper and lower nibbles of register. */
static void
gb_swap(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t v = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t l = (uint8_t) ((v & 0x0F) << 4);
    uint8_t h = (uint8_t) ((v & 0xF0) >> 4);
    uint8_t r = l | h;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = 0;

    gb_cpu_set(cpu, bus, op->arg1, r);
}

/* BIT b,r8
 * Flags: Z 0 1 -
 * Test bit b in 8-bit register or memory location. */
static void
gb_bit(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t bit = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    uint8_t v = gb_cpu_getbit(cpu, bus, op->arg2, bit);

    cpu->flags.zero = v == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 1;
}

/* SET b,r8
 * Flags: - - - -
 * Set bit b in 8-bit register or memory location. */
static void
gb_set(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t bit = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    gb_cpu_setbit(cpu, bus, op->arg2, bit, 1);
}

/* RES b,r8
 * Flags: - - - -
 * Reset bit b in 8-bit register or memory location. */
static void
gb_res(gb_cpu_t *cpu, gb_bus_t *bus, const gb_instr_t *op)
{
    uint8_t bit = (uint8_t) gb_cpu_get(cpu, bus, op->arg1);
    gb_cpu_setbit(cpu, bus, op->arg2, bit, 0);
}

/* ----------------------------------------------------------------------------
 * Opcode Tables
 * -------------------------------------------------------------------------- */

#define gb_op(op, arg1, arg2, handler, cost, text) \
    [op] = {op, cost, arg1, arg2, handler, text}

const gb_instr_t gb_opcodes[256] = {
    gb_op(0x00, ARG_NONE, ARG_NONE, gb_nop, 1, "NOP"),
    gb_op(0xF3, ARG_NONE, ARG_NONE, gb_di, 1, "DI"),
    gb_op(0xFB, ARG_NONE, ARG_NONE, gb_ei, 1, "EI"),
    gb_op(0x27, ARG_NONE, ARG_NONE, gb_daa, 1, "DAA"),
    gb_op(0x2F, ARG_NONE, ARG_NONE, gb_cpl, 1, "CPL"),
    gb_op(0x37, ARG_NONE, ARG_NONE, gb_scf, 1, "SCF"),
    gb_op(0x3F, ARG_NONE, ARG_NONE, gb_ccf, 1, "CCF"),

    gb_op(0x04, ARG_REG_B, ARG_NONE, gb_inc8, 1, "INC B"),
    gb_op(0x0C, ARG_REG_C, ARG_NONE, gb_inc8, 1, "INC C"),
    gb_op(0x14, ARG_REG_D, ARG_NONE, gb_inc8, 1, "INC D"),
    gb_op(0x1C, ARG_REG_E, ARG_NONE, gb_inc8, 1, "INC E"),
    gb_op(0x24, ARG_REG_H, ARG_NONE, gb_inc8, 1, "INC H"),
    gb_op(0x2C, ARG_REG_L, ARG_NONE, gb_inc8, 1, "INC L"),
    gb_op(0x3C, ARG_REG_A, ARG_NONE, gb_inc8, 1, "INC A"),
    gb_op(0x34, ARG_IND_HL, ARG_NONE, gb_inc8, 3, "INC (HL)"),

    gb_op(0x03, ARG_REG_BC, ARG_NONE, gb_inc16, 2, "INC BC"),
    gb_op(0x13, ARG_REG_DE, ARG_NONE, gb_inc16, 2, "INC DE"),
    gb_op(0x23, ARG_REG_HL, ARG_NONE, gb_inc16, 2, "INC HL"),
    gb_op(0x33, ARG_REG_SP, ARG_NONE, gb_inc16, 2, "INC SP"),

    gb_op(0x05, ARG_REG_B, ARG_NONE, gb_dec8, 1, "DEC B"),
    gb_op(0x0D, ARG_REG_C, ARG_NONE, gb_dec8, 1, "DEC C"),
    gb_op(0x15, ARG_REG_D, ARG_NONE, gb_dec8, 1, "DEC D"),
    gb_op(0x1D, ARG_REG_E, ARG_NONE, gb_dec8, 1, "DEC E"),
    gb_op(0x25, ARG_REG_H, ARG_NONE, gb_dec8, 1, "DEC H"),
    gb_op(0x2D, ARG_REG_L, ARG_NONE, gb_dec8, 1, "DEC L"),
    gb_op(0x3D, ARG_REG_A, ARG_NONE, gb_dec8, 1, "DEC A"),
    gb_op(0x35, ARG_IND_HL, ARG_NONE, gb_dec8, 3, "DEC (HL)"),

    gb_op(0x0B, ARG_REG_BC, ARG_NONE, gb_dec16, 2, "DEC BC"),
    gb_op(0x1B, ARG_REG_DE, ARG_NONE, gb_dec16, 2, "DEC DE"),
    gb_op(0x2B, ARG_REG_HL, ARG_NONE, gb_dec16, 2, "DEC HL"),
    gb_op(0x3B, ARG_REG_SP, ARG_NONE, gb_dec16, 2, "DEC SP"),

    gb_op(0x02, ARG_IND_BC, ARG_REG_A, gb_ld8, 2, "LD (BC),A"),
    gb_op(0x0A, ARG_REG_A, ARG_IND_BC, gb_ld8, 2, "LD A,(BC)"),
    gb_op(0x12, ARG_IND_DE, ARG_REG_A, gb_ld8, 2, "LD (DE),A"),
    gb_op(0x1A, ARG_REG_A, ARG_IND_DE, gb_ld8, 2, "LD A,(DE)"),
    gb_op(0x22, ARG_IND_HLI, ARG_REG_A, gb_ld8, 2, "LD (HL+),A"),
    gb_op(0x2A, ARG_REG_A, ARG_IND_HLI, gb_ld8, 2, "LD A,(HL+)"),
    gb_op(0x32, ARG_IND_HLD, ARG_REG_A, gb_ld8, 2, "LD (HL-),A"),
    gb_op(0x3A, ARG_REG_A, ARG_IND_HLD, gb_ld8, 2, "LD A,(HL-)"),
    gb_op(0x40, ARG_REG_B, ARG_REG_B, gb_ld8, 1, "LD B,B"),
    gb_op(0x41, ARG_REG_B, ARG_REG_C, gb_ld8, 1, "LD B,C"),
    gb_op(0x42, ARG_REG_B, ARG_REG_D, gb_ld8, 1, "LD B,D"),
    gb_op(0x43, ARG_REG_B, ARG_REG_E, gb_ld8, 1, "LD B,E"),
    gb_op(0x44, ARG_REG_B, ARG_REG_H, gb_ld8, 1, "LD B,H"),
    gb_op(0x45, ARG_REG_B, ARG_REG_L, gb_ld8, 1, "LD B,L"),
    gb_op(0x46, ARG_REG_B, ARG_IND_HL, gb_ld8, 2, "LD B,(HL)"),
    gb_op(0x47, ARG_REG_B, ARG_REG_A, gb_ld8, 1, "LD B,A"),
    gb_op(0x48, ARG_REG_C, ARG_REG_B, gb_ld8, 1, "LD C,B"),
    gb_op(0x49, ARG_REG_C, ARG_REG_C, gb_ld8, 1, "LD C,C"),
    gb_op(0x4A, ARG_REG_C, ARG_REG_D, gb_ld8, 1, "LD C,D"),
    gb_op(0x4B, ARG_REG_C, ARG_REG_E, gb_ld8, 1, "LD C,E"),
    gb_op(0x4C, ARG_REG_C, ARG_REG_H, gb_ld8, 1, "LD C,H"),
    gb_op(0x4D, ARG_REG_C, ARG_REG_L, gb_ld8, 1, "LD C,L"),
    gb_op(0x4E, ARG_REG_C, ARG_IND_HL, gb_ld8, 2, "LD C,(HL)"),
    gb_op(0x4F, ARG_REG_C, ARG_REG_A, gb_ld8, 1, "LD C,A"),
    gb_op(0x50, ARG_REG_D, ARG_REG_B, gb_ld8, 1, "LD D,B"),
    gb_op(0x51, ARG_REG_D, ARG_REG_C, gb_ld8, 1, "LD D,C"),
    gb_op(0x52, ARG_REG_D, ARG_REG_D, gb_ld8, 1, "LD D,D"),
    gb_op(0x53, ARG_REG_D, ARG_REG_E, gb_ld8, 1, "LD D,E"),
    gb_op(0x54, ARG_REG_D, ARG_REG_H, gb_ld8, 1, "LD D,H"),
    gb_op(0x55, ARG_REG_D, ARG_REG_L, gb_ld8, 1, "LD D,L"),
    gb_op(0x56, ARG_REG_D, ARG_IND_HL, gb_ld8, 2, "LD D,(HL)"),
    gb_op(0x57, ARG_REG_D, ARG_REG_A, gb_ld8, 1, "LD D,A"),
    gb_op(0x58, ARG_REG_E, ARG_REG_B, gb_ld8, 1, "LD E,B"),
    gb_op(0x59, ARG_REG_E, ARG_REG_C, gb_ld8, 1, "LD E,C"),
    gb_op(0x5A, ARG_REG_E, ARG_REG_D, gb_ld8, 1, "LD E,D"),
    gb_op(0x5B, ARG_REG_E, ARG_REG_E, gb_ld8, 1, "LD E,E"),
    gb_op(0x5C, ARG_REG_E, ARG_REG_H, gb_ld8, 1, "LD E,H"),
    gb_op(0x5D, ARG_REG_E, ARG_REG_L, gb_ld8, 1, "LD E,L"),
    gb_op(0x5E, ARG_REG_E, ARG_IND_HL, gb_ld8, 2, "LD E,(HL)"),
    gb_op(0x5F, ARG_REG_E, ARG_REG_A, gb_ld8, 1, "LD E,A"),
    gb_op(0x60, ARG_REG_H, ARG_REG_B, gb_ld8, 1, "LD H,B"),
    gb_op(0x61, ARG_REG_H, ARG_REG_C, gb_ld8, 1, "LD H,C"),
    gb_op(0x62, ARG_REG_H, ARG_REG_D, gb_ld8, 1, "LD H,D"),
    gb_op(0x63, ARG_REG_H, ARG_REG_E, gb_ld8, 1, "LD H,E"),
    gb_op(0x64, ARG_REG_H, ARG_REG_H, gb_ld8, 1, "LD H,H"),
    gb_op(0x65, ARG_REG_H, ARG_REG_L, gb_ld8, 1, "LD H,L"),
    gb_op(0x66, ARG_REG_H, ARG_IND_HL, gb_ld8, 2, "LD H,(HL)"),
    gb_op(0x67, ARG_REG_H, ARG_REG_A, gb_ld8, 1, "LD H,A"),
    gb_op(0x68, ARG_REG_L, ARG_REG_B, gb_ld8, 1, "LD L,B"),
    gb_op(0x69, ARG_REG_L, ARG_REG_C, gb_ld8, 1, "LD L,C"),
    gb_op(0x6A, ARG_REG_L, ARG_REG_D, gb_ld8, 1, "LD L,D"),
    gb_op(0x6B, ARG_REG_L, ARG_REG_E, gb_ld8, 1, "LD L,E"),
    gb_op(0x6C, ARG_REG_L, ARG_REG_H, gb_ld8, 1, "LD L,H"),
    gb_op(0x6D, ARG_REG_L, ARG_REG_L, gb_ld8, 1, "LD L,L"),
    gb_op(0x6E, ARG_REG_L, ARG_IND_HL, gb_ld8, 2, "LD L,(HL)"),
    gb_op(0x6F, ARG_REG_L, ARG_REG_A, gb_ld8, 1, "LD L,A"),
    gb_op(0x70, ARG_IND_HL, ARG_REG_B, gb_ld8, 2, "LD (HL),B"),
    gb_op(0x71, ARG_IND_HL, ARG_REG_C, gb_ld8, 2, "LD (HL),C"),
    gb_op(0x72, ARG_IND_HL, ARG_REG_D, gb_ld8, 2, "LD (HL),D"),
    gb_op(0x73, ARG_IND_HL, ARG_REG_E, gb_ld8, 2, "LD (HL),E"),
    gb_op(0x74, ARG_IND_HL, ARG_REG_H, gb_ld8, 2, "LD (HL),H"),
    gb_op(0x75, ARG_IND_HL, ARG_REG_L, gb_ld8, 2, "LD (HL),L"),
    gb_op(0x77, ARG_IND_HL, ARG_REG_A, gb_ld8, 2, "LD (HL),A"),
    gb_op(0x78, ARG_REG_A, ARG_REG_B, gb_ld8, 1, "LD A,B"),
    gb_op(0x79, ARG_REG_A, ARG_REG_C, gb_ld8, 1, "LD A,C"),
    gb_op(0x7A, ARG_REG_A, ARG_REG_D, gb_ld8, 1, "LD A,D"),
    gb_op(0x7B, ARG_REG_A, ARG_REG_E, gb_ld8, 1, "LD A,E"),
    gb_op(0x7C, ARG_REG_A, ARG_REG_H, gb_ld8, 1, "LD A,H"),
    gb_op(0x7D, ARG_REG_A, ARG_REG_L, gb_ld8, 1, "LD A,L"),
    gb_op(0x7E, ARG_REG_A, ARG_IND_HL, gb_ld8, 2, "LD A,(HL)"),
    gb_op(0x7F, ARG_REG_A, ARG_REG_A, gb_ld8, 1, "LD A,A"),
    gb_op(0xE0, ARG_IND_IMM8, ARG_REG_A, gb_ld8, 3, "LD ($%02X),A"),
    gb_op(0xF0, ARG_REG_A, ARG_IND_IMM8, gb_ld8, 3, "LD A,($%02X)"),
    gb_op(0xEA, ARG_IND_IMM16, ARG_REG_A, gb_ld8, 4, "LD ($%04X),A"),
    gb_op(0xFA, ARG_REG_A, ARG_IND_IMM16, gb_ld8, 4, "LD A,($%04X)"),
    gb_op(0x06, ARG_REG_B, ARG_IMM8, gb_ld8, 2, "LD B,$%02X"),
    gb_op(0xE2, ARG_IND_C, ARG_REG_A, gb_ld8, 2, "LD (C),A"),
    gb_op(0xF2, ARG_REG_C, ARG_IMM8, gb_ld8, 2, "LD C,$%02X"),
    gb_op(0x0E, ARG_REG_C, ARG_IMM8, gb_ld8, 2, "LD C,$%02X"),
    gb_op(0x16, ARG_REG_D, ARG_IMM8, gb_ld8, 2, "LD D,$%02X"),
    gb_op(0x1E, ARG_REG_E, ARG_IMM8, gb_ld8, 2, "LD E,$%02X"),
    gb_op(0x26, ARG_REG_H, ARG_IMM8, gb_ld8, 2, "LD H,$%02X"),
    gb_op(0x2E, ARG_REG_L, ARG_IMM8, gb_ld8, 2, "LD L,$%02X"),
    gb_op(0x36, ARG_IND_HL, ARG_IMM8, gb_ld8, 3, "LD (HL),$%02X"),
    gb_op(0x3E, ARG_REG_A, ARG_IMM8, gb_ld8, 2, "LD A,$%02X"),

    gb_op(0x01, ARG_REG_BC, ARG_IMM16, gb_ld16, 3, "LD BC,$%04X"),
    gb_op(0x11, ARG_REG_DE, ARG_IMM16, gb_ld16, 3, "LD DE,$%04X"),
    gb_op(0x21, ARG_REG_HL, ARG_IMM16, gb_ld16, 3, "LD HL,$%04X"),
    gb_op(0x31, ARG_REG_SP, ARG_IMM16, gb_ld16, 3, "LD SP,$%04X"),
    gb_op(0x08, ARG_IND_IMM16, ARG_REG_SP, gb_ld16, 5, "LD ($%04X),SP"),
    gb_op(0xF9, ARG_REG_SP, ARG_REG_HL, gb_ld16, 2, "LD SP,HL"),

    gb_op(0x80, ARG_REG_B, ARG_NONE, gb_add_a, 1, "ADD A,B"),
    gb_op(0x81, ARG_REG_C, ARG_NONE, gb_add_a, 1, "ADD A,C"),
    gb_op(0x82, ARG_REG_D, ARG_NONE, gb_add_a, 1, "ADD A,D"),
    gb_op(0x83, ARG_REG_E, ARG_NONE, gb_add_a, 1, "ADD A,E"),
    gb_op(0x84, ARG_REG_H, ARG_NONE, gb_add_a, 1, "ADD A,H"),
    gb_op(0x85, ARG_REG_L, ARG_NONE, gb_add_a, 1, "ADD A,L"),
    gb_op(0x86, ARG_IND_HL, ARG_NONE, gb_add_a, 2, "ADD A,(HL)"),
    gb_op(0x87, ARG_REG_A, ARG_NONE, gb_add_a, 1, "ADD A,A"),
    gb_op(0xC6, ARG_IMM8, ARG_NONE, gb_add_a, 2, "ADD A,$%02X"),

    gb_op(0xE8, ARG_IMM8, ARG_NONE, gb_add_sp, 4, "ADD SP,$%02X"),
    gb_op(0x09, ARG_REG_BC, ARG_NONE, gb_add_hl, 2, "ADD HL,BC"),
    gb_op(0x19, ARG_REG_DE, ARG_NONE, gb_add_hl, 2, "ADD HL,DE"),
    gb_op(0x29, ARG_REG_HL, ARG_NONE, gb_add_hl, 2, "ADD HL,HL"),
    gb_op(0x39, ARG_REG_SP, ARG_NONE, gb_add_hl, 2, "ADD HL,SP"),

    gb_op(0x88, ARG_REG_B, ARG_NONE, gb_adc8, 1, "ADC A,B"),
    gb_op(0x89, ARG_REG_C, ARG_NONE, gb_adc8, 1, "ADC A,C"),
    gb_op(0x8A, ARG_REG_D, ARG_NONE, gb_adc8, 1, "ADC A,D"),
    gb_op(0x8B, ARG_REG_E, ARG_NONE, gb_adc8, 1, "ADC A,E"),
    gb_op(0x8C, ARG_REG_H, ARG_NONE, gb_adc8, 1, "ADC A,H"),
    gb_op(0x8D, ARG_REG_L, ARG_NONE, gb_adc8, 1, "ADC A,L"),
    gb_op(0x8E, ARG_IND_HL, ARG_NONE, gb_adc8, 2, "ADC A,(HL)"),
    gb_op(0x8F, ARG_REG_A, ARG_NONE, gb_adc8, 1, "ADC A,A"),
    gb_op(0xCE, ARG_IMM8, ARG_NONE, gb_adc8, 2, "ADC A,$%02X"),

    gb_op(0x90, ARG_REG_B, ARG_NONE, gb_sub8, 1, "SUB A,B"),
    gb_op(0x91, ARG_REG_C, ARG_NONE, gb_sub8, 1, "SUB A,C"),
    gb_op(0x92, ARG_REG_D, ARG_NONE, gb_sub8, 1, "SUB A,D"),
    gb_op(0x93, ARG_REG_E, ARG_NONE, gb_sub8, 1, "SUB A,E"),
    gb_op(0x94, ARG_REG_H, ARG_NONE, gb_sub8, 1, "SUB A,H"),
    gb_op(0x95, ARG_REG_L, ARG_NONE, gb_sub8, 1, "SUB A,L"),
    gb_op(0x96, ARG_IND_HL, ARG_NONE, gb_sub8, 2, "SUB A,(HL)"),
    gb_op(0x97, ARG_REG_A, ARG_NONE, gb_sub8, 1, "SUB A,A"),
    gb_op(0xD6, ARG_IMM8, ARG_NONE, gb_sub8, 2, "SUB A,$%02X"),

    gb_op(0x98, ARG_REG_B, ARG_NONE, gb_sbc8, 1, "SBC A,B"),
    gb_op(0x99, ARG_REG_C, ARG_NONE, gb_sbc8, 1, "SBC A,C"),
    gb_op(0x9A, ARG_REG_D, ARG_NONE, gb_sbc8, 1, "SBC A,D"),
    gb_op(0x9B, ARG_REG_E, ARG_NONE, gb_sbc8, 1, "SBC A,E"),
    gb_op(0x9C, ARG_REG_H, ARG_NONE, gb_sbc8, 1, "SBC A,H"),
    gb_op(0x9D, ARG_REG_L, ARG_NONE, gb_sbc8, 1, "SBC A,L"),
    gb_op(0x9E, ARG_IND_HL, ARG_NONE, gb_sbc8, 2, "SBC A,(HL)"),
    gb_op(0x9F, ARG_REG_A, ARG_NONE, gb_sbc8, 1, "SBC A,A"),
    gb_op(0xDE, ARG_IMM8, ARG_NONE, gb_sbc8, 2, "SBC A,$%02X"),

    gb_op(0xA0, ARG_REG_B, ARG_NONE, gb_and8, 1, "AND B"),
    gb_op(0xA1, ARG_REG_C, ARG_NONE, gb_and8, 1, "AND C"),
    gb_op(0xA2, ARG_REG_D, ARG_NONE, gb_and8, 1, "AND D"),
    gb_op(0xA3, ARG_REG_E, ARG_NONE, gb_and8, 1, "AND E"),
    gb_op(0xA4, ARG_REG_H, ARG_NONE, gb_and8, 1, "AND H"),
    gb_op(0xA5, ARG_REG_L, ARG_NONE, gb_and8, 1, "AND L"),
    gb_op(0xA6, ARG_IND_HL, ARG_NONE, gb_and8, 2, "AND (HL)"),
    gb_op(0xA7, ARG_REG_A, ARG_NONE, gb_and8, 1, "AND A"),
    gb_op(0xE6, ARG_IMM8, ARG_NONE, gb_and8, 2, "AND $%02X"),

    gb_op(0xA8, ARG_REG_B, ARG_NONE, gb_xor8, 1, "XOR B"),
    gb_op(0xA9, ARG_REG_C, ARG_NONE, gb_xor8, 1, "XOR C"),
    gb_op(0xAA, ARG_REG_D, ARG_NONE, gb_xor8, 1, "XOR D"),
    gb_op(0xAB, ARG_REG_E, ARG_NONE, gb_xor8, 1, "XOR E"),
    gb_op(0xAC, ARG_REG_H, ARG_NONE, gb_xor8, 1, "XOR H"),
    gb_op(0xAD, ARG_REG_L, ARG_NONE, gb_xor8, 1, "XOR L"),
    gb_op(0xAE, ARG_IND_HL, ARG_NONE, gb_xor8, 2, "XOR (HL)"),
    gb_op(0xAF, ARG_REG_A, ARG_NONE, gb_xor8, 1, "XOR A"),
    gb_op(0xEE, ARG_IMM8, ARG_NONE, gb_xor8, 2, "XOR $%02X"),

    gb_op(0xB0, ARG_REG_B, ARG_NONE, gb_or8, 1, "OR B"),
    gb_op(0xB1, ARG_REG_C, ARG_NONE, gb_or8, 1, "OR C"),
    gb_op(0xB2, ARG_REG_D, ARG_NONE, gb_or8, 1, "OR D"),
    gb_op(0xB3, ARG_REG_E, ARG_NONE, gb_or8, 1, "OR E"),
    gb_op(0xB4, ARG_REG_H, ARG_NONE, gb_or8, 1, "OR H"),
    gb_op(0xB5, ARG_REG_L, ARG_NONE, gb_or8, 1, "OR L"),
    gb_op(0xB6, ARG_IND_HL, ARG_NONE, gb_or8, 2, "OR (HL)"),
    gb_op(0xB7, ARG_REG_A, ARG_NONE, gb_or8, 1, "OR A"),
    gb_op(0xF6, ARG_IMM8, ARG_NONE, gb_or8, 2, "OR $%02X"),

    gb_op(0xB8, ARG_REG_B, ARG_NONE, gb_cp8, 1, "CP B"),
    gb_op(0xB9, ARG_REG_C, ARG_NONE, gb_cp8, 1, "CP C"),
    gb_op(0xBA, ARG_REG_D, ARG_NONE, gb_cp8, 1, "CP D"),
    gb_op(0xBB, ARG_REG_E, ARG_NONE, gb_cp8, 1, "CP E"),
    gb_op(0xBC, ARG_REG_H, ARG_NONE, gb_cp8, 1, "CP H"),
    gb_op(0xBD, ARG_REG_L, ARG_NONE, gb_cp8, 1, "CP L"),
    gb_op(0xBE, ARG_IND_HL, ARG_NONE, gb_cp8, 2, "CP (HL)"),
    gb_op(0xBF, ARG_REG_A, ARG_NONE, gb_cp8, 1, "CP A"),
    gb_op(0xFE, ARG_IMM8, ARG_NONE, gb_cp8, 2, "CP $%02X"),

    gb_op(0xC1, ARG_REG_BC, ARG_NONE, gb_pop16, 3, "POP BC"),
    gb_op(0xD1, ARG_REG_DE, ARG_NONE, gb_pop16, 3, "POP DE"),
    gb_op(0xE1, ARG_REG_HL, ARG_NONE, gb_pop16, 3, "POP HL"),
    gb_op(0xF1, ARG_REG_AF, ARG_NONE, gb_pop16, 3, "POP AF"),

    gb_op(0xC5, ARG_REG_BC, ARG_NONE, gb_push16, 4, "PUSH BC"),
    gb_op(0xD5, ARG_REG_DE, ARG_NONE, gb_push16, 4, "PUSH DE"),
    gb_op(0xE5, ARG_REG_HL, ARG_NONE, gb_push16, 4, "PUSH HL"),
    gb_op(0xF5, ARG_REG_AF, ARG_NONE, gb_push16, 4, "PUSH AF"),

    gb_op(0x18, ARG_IMM8, ARG_NONE, gb_jr8, 2, "JR $%02X"),
    gb_op(0x28, ARG_FLAG_ZERO, ARG_IMM8, gb_jr8_if, 2, "JR Z,$%02X"),
    gb_op(0x38, ARG_FLAG_CARRY, ARG_IMM8, gb_jr8_if, 2, "JR C,$%02X"),
    gb_op(0x20, ARG_FLAG_ZERO, ARG_IMM8, gb_jr8_ifn, 2, "JR NZ,$%02X"),
    gb_op(0x30, ARG_FLAG_CARRY, ARG_IMM8, gb_jr8_ifn, 2, "JR NC,$%02X"),

    gb_op(0xC3, ARG_IMM16, ARG_NONE, gb_jp16, 4, "JP $%04X"),
    gb_op(0xE9, ARG_REG_HL, ARG_NONE, gb_jp16, 1, "JP HL"),
    gb_op(0xCA, ARG_FLAG_ZERO, ARG_IMM16, gb_jp16_if, 3, "JP Z,$%04X"),
    gb_op(0xDA, ARG_FLAG_CARRY, ARG_IMM16, gb_jp16_if, 3, "JP C,$%04X"),
    gb_op(0xC2, ARG_FLAG_ZERO, ARG_IMM16, gb_jp16_ifn, 3, "JP NZ,$%04X"),
    gb_op(0xD2, ARG_FLAG_CARRY, ARG_IMM16, gb_jp16_ifn, 3, "JP NC,$%04X"),

    gb_op(0xCD, ARG_IMM16, ARG_NONE, gb_call, 6, "CALL $%04X"),
    gb_op(0xCC, ARG_FLAG_ZERO, ARG_IMM16, gb_call_if, 3, "CALL Z,$%04X"),
    gb_op(0xDC, ARG_FLAG_CARRY, ARG_IMM16, gb_call_if, 3, "CALL C,$%04X"),
    gb_op(0xC4, ARG_FLAG_ZERO, ARG_IMM16, gb_call_ifn, 3, "CALL NZ,$%04X"),
    gb_op(0xD4, ARG_FLAG_CARRY, ARG_IMM16, gb_call_ifn, 3, "CALL NC,$%04X"),

    gb_op(0xC9, ARG_NONE, ARG_NONE, gb_ret, 4, "RET"),
    gb_op(0xD9, ARG_NONE, ARG_NONE, gb_reti, 4, "RETI"),
    gb_op(0xC8, ARG_FLAG_ZERO, ARG_NONE, gb_ret_if, 2, "RET Z"),
    gb_op(0xD8, ARG_FLAG_CARRY, ARG_NONE, gb_ret_if, 2, "RET C"),
    gb_op(0xC0, ARG_FLAG_ZERO, ARG_NONE, gb_ret_ifn, 2, "RET NZ"),
    gb_op(0xD0, ARG_FLAG_CARRY, ARG_NONE, gb_ret_ifn, 2, "RET NC"),

    gb_op(0xC7, ARG_RST_0, ARG_NONE, gb_rst, 4, "RST 0"),
    gb_op(0xCF, ARG_RST_1, ARG_NONE, gb_rst, 4, "RST 1"),
    gb_op(0xD7, ARG_RST_2, ARG_NONE, gb_rst, 4, "RST 2"),
    gb_op(0xDF, ARG_RST_3, ARG_NONE, gb_rst, 4, "RST 3"),
    gb_op(0xE7, ARG_RST_4, ARG_NONE, gb_rst, 4, "RST 4"),
    gb_op(0xEF, ARG_RST_5, ARG_NONE, gb_rst, 4, "RST 5"),
    gb_op(0xF7, ARG_RST_6, ARG_NONE, gb_rst, 4, "RST 6"),
    gb_op(0xFF, ARG_RST_7, ARG_NONE, gb_rst, 4, "RST 7"),

    gb_op(0x07, ARG_REG_A, ARG_NONE, gb_rlc, 1, "RLCA"),
    gb_op(0x17, ARG_REG_A, ARG_NONE, gb_rla, 1, "RLA"),
    gb_op(0x0F, ARG_REG_A, ARG_NONE, gb_rrca, 1, "RRCA"),
    gb_op(0x1F, ARG_REG_A, ARG_NONE, gb_rra, 1, "RRA"),
};

const gb_instr_t gb_cb_opcodes[256] = {
    gb_op(0x00, ARG_REG_B, ARG_NONE, gb_rlc, 2, "RLC B"),
    gb_op(0x01, ARG_REG_C, ARG_NONE, gb_rlc, 2, "RLC C"),
    gb_op(0x02, ARG_REG_D, ARG_NONE, gb_rlc, 2, "RLC D"),
    gb_op(0x03, ARG_REG_E, ARG_NONE, gb_rlc, 2, "RLC E"),
    gb_op(0x04, ARG_REG_H, ARG_NONE, gb_rlc, 2, "RLC H"),
    gb_op(0x05, ARG_REG_L, ARG_NONE, gb_rlc, 2, "RLC L"),
    gb_op(0x06, ARG_IND_HL, ARG_NONE, gb_rlc, 4, "RLC (HL)"),
    gb_op(0x07, ARG_REG_A, ARG_NONE, gb_rlc, 2, "RLC A"),

    gb_op(0x08, ARG_REG_B, ARG_NONE, gb_rrc, 2, "RRC B"),
    gb_op(0x09, ARG_REG_C, ARG_NONE, gb_rrc, 2, "RRC C"),
    gb_op(0x0A, ARG_REG_D, ARG_NONE, gb_rrc, 2, "RRC D"),
    gb_op(0x0B, ARG_REG_E, ARG_NONE, gb_rrc, 2, "RRC E"),
    gb_op(0x0C, ARG_REG_H, ARG_NONE, gb_rrc, 2, "RRC H"),
    gb_op(0x0D, ARG_REG_L, ARG_NONE, gb_rrc, 2, "RRC L"),
    gb_op(0x0E, ARG_IND_HL, ARG_NONE, gb_rrc, 4, "RRC (HL)"),
    gb_op(0x0F, ARG_REG_A, ARG_NONE, gb_rrc, 2, "RRC A"),

    gb_op(0x10, ARG_REG_B, ARG_NONE, gb_rl, 2, "RL B"),
    gb_op(0x11, ARG_REG_C, ARG_NONE, gb_rl, 2, "RL C"),
    gb_op(0x12, ARG_REG_D, ARG_NONE, gb_rl, 2, "RL D"),
    gb_op(0x13, ARG_REG_E, ARG_NONE, gb_rl, 2, "RL E"),
    gb_op(0x14, ARG_REG_H, ARG_NONE, gb_rl, 2, "RL H"),
    gb_op(0x15, ARG_REG_L, ARG_NONE, gb_rl, 2, "RL L"),
    gb_op(0x16, ARG_IND_HL, ARG_NONE, gb_rl, 4, "RL (HL)"),
    gb_op(0x17, ARG_REG_A, ARG_NONE, gb_rl, 2, "RL A"),

    gb_op(0x18, ARG_REG_B, ARG_NONE, gb_rr, 2, "RR B"),
    gb_op(0x19, ARG_REG_C, ARG_NONE, gb_rr, 2, "RR C"),
    gb_op(0x1A, ARG_REG_D, ARG_NONE, gb_rr, 2, "RR D"),
    gb_op(0x1B, ARG_REG_E, ARG_NONE, gb_rr, 2, "RR E"),
    gb_op(0x1C, ARG_REG_H, ARG_NONE, gb_rr, 2, "RR H"),
    gb_op(0x1D, ARG_REG_L, ARG_NONE, gb_rr, 2, "RR L"),
    gb_op(0x1E, ARG_IND_HL, ARG_NONE, gb_rr, 4, "RR (HL)"),
    gb_op(0x1F, ARG_REG_A, ARG_NONE, gb_rr, 2, "RR A"),

    gb_op(0x20, ARG_REG_B, ARG_NONE, gb_sla, 2, "SLA B"),
    gb_op(0x21, ARG_REG_C, ARG_NONE, gb_sla, 2, "SLA C"),
    gb_op(0x22, ARG_REG_D, ARG_NONE, gb_sla, 2, "SLA D"),
    gb_op(0x23, ARG_REG_E, ARG_NONE, gb_sla, 2, "SLA E"),
    gb_op(0x24, ARG_REG_H, ARG_NONE, gb_sla, 2, "SLA H"),
    gb_op(0x25, ARG_REG_L, ARG_NONE, gb_sla, 2, "SLA L"),
    gb_op(0x26, ARG_IND_HL, ARG_NONE, gb_sla, 4, "SLA (HL)"),
    gb_op(0x27, ARG_REG_A, ARG_NONE, gb_sla, 2, "SLA A"),

    gb_op(0x28, ARG_REG_B, ARG_NONE, gb_sra, 2, "SRA B"),
    gb_op(0x29, ARG_REG_C, ARG_NONE, gb_sra, 2, "SRA C"),
    gb_op(0x2A, ARG_REG_D, ARG_NONE, gb_sra, 2, "SRA D"),
    gb_op(0x2B, ARG_REG_E, ARG_NONE, gb_sra, 2, "SRA E"),
    gb_op(0x2C, ARG_REG_H, ARG_NONE, gb_sra, 2, "SRA H"),
    gb_op(0x2D, ARG_REG_L, ARG_NONE, gb_sra, 2, "SRA L"),
    gb_op(0x2E, ARG_IND_HL, ARG_NONE, gb_sra, 4, "SRA (HL)"),
    gb_op(0x2F, ARG_REG_A, ARG_NONE, gb_sra, 2, "SRA A"),

    gb_op(0x30, ARG_REG_B, ARG_NONE, gb_swap, 2, "SWAP B"),
    gb_op(0x31, ARG_REG_C, ARG_NONE, gb_swap, 2, "SWAP C"),
    gb_op(0x32, ARG_REG_D, ARG_NONE, gb_swap, 2, "SWAP D"),
    gb_op(0x33, ARG_REG_E, ARG_NONE, gb_swap, 2, "SWAP E"),
    gb_op(0x34, ARG_REG_H, ARG_NONE, gb_swap, 2, "SWAP H"),
    gb_op(0x35, ARG_REG_L, ARG_NONE, gb_swap, 2, "SWAP L"),
    gb_op(0x36, ARG_IND_HL, ARG_NONE, gb_swap, 4, "SWAP (HL)"),
    gb_op(0x37, ARG_REG_A, ARG_NONE, gb_swap, 2, "SWAP A"),

    gb_op(0x38, ARG_REG_B, ARG_NONE, gb_srl, 2, "SRL B"),
    gb_op(0x39, ARG_REG_C, ARG_NONE, gb_srl, 2, "SRL C"),
    gb_op(0x3A, ARG_REG_D, ARG_NONE, gb_srl, 2, "SRL D"),
    gb_op(0x3B, ARG_REG_E, ARG_NONE, gb_srl, 2, "SRL E"),
    gb_op(0x3C, ARG_REG_H, ARG_NONE, gb_srl, 2, "SRL H"),
    gb_op(0x3D, ARG_REG_L, ARG_NONE, gb_srl, 2, "SRL L"),
    gb_op(0x3E, ARG_IND_HL, ARG_NONE, gb_srl, 4, "SRL (HL)"),
    gb_op(0x3F, ARG_REG_A, ARG_NONE, gb_srl, 2, "SRL A"),

    gb_op(0x40, ARG_BIT_0, ARG_REG_B, gb_bit, 2, "BIT 0,B"),
    gb_op(0x41, ARG_BIT_0, ARG_REG_C, gb_bit, 2, "BIT 0,C"),
    gb_op(0x42, ARG_BIT_0, ARG_REG_D, gb_bit, 2, "BIT 0,D"),
    gb_op(0x43, ARG_BIT_0, ARG_REG_E, gb_bit, 2, "BIT 0,E"),
    gb_op(0x44, ARG_BIT_0, ARG_REG_H, gb_bit, 2, "BIT 0,H"),
    gb_op(0x45, ARG_BIT_0, ARG_REG_L, gb_bit, 2, "BIT 0,L"),
    gb_op(0x46, ARG_BIT_0, ARG_IND_HL, gb_bit, 3, "BIT 0,(HL)"),
    gb_op(0x47, ARG_BIT_0, ARG_REG_A, gb_bit, 2, "BIT 0,A"),
    gb_op(0x48, ARG_BIT_1, ARG_REG_B, gb_bit, 2, "BIT 1,B"),
    gb_op(0x49, ARG_BIT_1, ARG_REG_C, gb_bit, 2, "BIT 1,C"),
    gb_op(0x4A, ARG_BIT_1, ARG_REG_D, gb_bit, 2, "BIT 1,D"),
    gb_op(0x4B, ARG_BIT_1, ARG_REG_E, gb_bit, 2, "BIT 1,E"),
    gb_op(0x4C, ARG_BIT_1, ARG_REG_H, gb_bit, 2, "BIT 1,H"),
    gb_op(0x4D, ARG_BIT_1, ARG_REG_L, gb_bit, 2, "BIT 1,L"),
    gb_op(0x4E, ARG_BIT_1, ARG_IND_HL, gb_bit, 3, "BIT 1,(HL)"),
    gb_op(0x4F, ARG_BIT_1, ARG_REG_A, gb_bit, 2, "BIT 1,A"),
    gb_op(0x50, ARG_BIT_2, ARG_REG_B, gb_bit, 2, "BIT 2,B"),
    gb_op(0x51, ARG_BIT_2, ARG_REG_C, gb_bit, 2, "BIT 2,C"),
    gb_op(0x52, ARG_BIT_2, ARG_REG_D, gb_bit, 2, "BIT 2,D"),
    gb_op(0x53, ARG_BIT_2, ARG_REG_E, gb_bit, 2, "BIT 2,E"),
    gb_op(0x54, ARG_BIT_2, ARG_REG_H, gb_bit, 2, "BIT 2,H"),
    gb_op(0x55, ARG_BIT_2, ARG_REG_L, gb_bit, 2, "BIT 2,L"),
    gb_op(0x56, ARG_BIT_2, ARG_IND_HL, gb_bit, 3, "BIT 2,(HL)"),
    gb_op(0x57, ARG_BIT_2, ARG_REG_A, gb_bit, 2, "BIT 2,A"),
    gb_op(0x58, ARG_BIT_3, ARG_REG_B, gb_bit, 2, "BIT 3,B"),
    gb_op(0x59, ARG_BIT_3, ARG_REG_C, gb_bit, 2, "BIT 3,C"),
    gb_op(0x5A, ARG_BIT_3, ARG_REG_D, gb_bit, 2, "BIT 3,D"),
    gb_op(0x5B, ARG_BIT_3, ARG_REG_E, gb_bit, 2, "BIT 3,E"),
    gb_op(0x5C, ARG_BIT_3, ARG_REG_H, gb_bit, 2, "BIT 3,H"),
    gb_op(0x5D, ARG_BIT_3, ARG_REG_L, gb_bit, 2, "BIT 3,L"),
    gb_op(0x5E, ARG_BIT_3, ARG_IND_HL, gb_bit, 3, "BIT 3,(HL)"),
    gb_op(0x5F, ARG_BIT_3, ARG_REG_A, gb_bit, 2, "BIT 3,A"),
    gb_op(0x60, ARG_BIT_4, ARG_REG_B, gb_bit, 2, "BIT 4,B"),
    gb_op(0x61, ARG_BIT_4, ARG_REG_C, gb_bit, 2, "BIT 4,C"),
    gb_op(0x62, ARG_BIT_4, ARG_REG_D, gb_bit, 2, "BIT 4,D"),
    gb_op(0x63, ARG_BIT_4, ARG_REG_E, gb_bit, 2, "BIT 4,E"),
    gb_op(0x64, ARG_BIT_4, ARG_REG_H, gb_bit, 2, "BIT 4,H"),
    gb_op(0x65, ARG_BIT_4, ARG_REG_L, gb_bit, 2, "BIT 4,L"),
    gb_op(0x66, ARG_BIT_4, ARG_IND_HL, gb_bit, 3, "BIT 4,(HL)"),
    gb_op(0x67, ARG_BIT_4, ARG_REG_A, gb_bit, 2, "BIT 4,A"),
    gb_op(0x68, ARG_BIT_5, ARG_REG_B, gb_bit, 2, "BIT 5,B"),
    gb_op(0x69, ARG_BIT_5, ARG_REG_C, gb_bit, 2, "BIT 5,C"),
    gb_op(0x6A, ARG_BIT_5, ARG_REG_D, gb_bit, 2, "BIT 5,D"),
    gb_op(0x6B, ARG_BIT_5, ARG_REG_E, gb_bit, 2, "BIT 5,E"),
    gb_op(0x6C, ARG_BIT_5, ARG_REG_H, gb_bit, 2, "BIT 5,H"),
    gb_op(0x6D, ARG_BIT_5, ARG_REG_L, gb_bit, 2, "BIT 5,L"),
    gb_op(0x6E, ARG_BIT_5, ARG_IND_HL, gb_bit, 3, "BIT 5,(HL)"),
    gb_op(0x6F, ARG_BIT_5, ARG_REG_A, gb_bit, 2, "BIT 5,A"),
    gb_op(0x70, ARG_BIT_6, ARG_REG_B, gb_bit, 2, "BIT 6,B"),
    gb_op(0x71, ARG_BIT_6, ARG_REG_C, gb_bit, 2, "BIT 6,C"),
    gb_op(0x72, ARG_BIT_6, ARG_REG_D, gb_bit, 2, "BIT 6,D"),
    gb_op(0x73, ARG_BIT_6, ARG_REG_E, gb_bit, 2, "BIT 6,E"),
    gb_op(0x74, ARG_BIT_6, ARG_REG_H, gb_bit, 2, "BIT 6,H"),
    gb_op(0x75, ARG_BIT_6, ARG_REG_L, gb_bit, 2, "BIT 6,L"),
    gb_op(0x76, ARG_BIT_6, ARG_IND_HL, gb_bit, 3, "BIT 6,(HL)"),
    gb_op(0x77, ARG_BIT_6, ARG_REG_A, gb_bit, 2, "BIT 6,A"),
    gb_op(0x78, ARG_BIT_7, ARG_REG_B, gb_bit, 2, "BIT 7,B"),
    gb_op(0x79, ARG_BIT_7, ARG_REG_C, gb_bit, 2, "BIT 7,C"),
    gb_op(0x7A, ARG_BIT_7, ARG_REG_D, gb_bit, 2, "BIT 7,D"),
    gb_op(0x7B, ARG_BIT_7, ARG_REG_E, gb_bit, 2, "BIT 7,E"),
    gb_op(0x7C, ARG_BIT_7, ARG_REG_H, gb_bit, 2, "BIT 7,H"),
    gb_op(0x7D, ARG_BIT_7, ARG_REG_L, gb_bit, 2, "BIT 7,L"),
    gb_op(0x7E, ARG_BIT_7, ARG_IND_HL, gb_bit, 3, "BIT 7,(HL)"),
    gb_op(0x7F, ARG_BIT_7, ARG_REG_A, gb_bit, 2, "BIT 7,A"),

    gb_op(0x80, ARG_BIT_0, ARG_REG_B, gb_res, 2, "RES 0,B"),
    gb_op(0x81, ARG_BIT_0, ARG_REG_C, gb_res, 2, "RES 0,C"),
    gb_op(0x82, ARG_BIT_0, ARG_REG_D, gb_res, 2, "RES 0,D"),
    gb_op(0x83, ARG_BIT_0, ARG_REG_E, gb_res, 2, "RES 0,E"),
    gb_op(0x84, ARG_BIT_0, ARG_REG_H, gb_res, 2, "RES 0,H"),
    gb_op(0x85, ARG_BIT_0, ARG_REG_L, gb_res, 2, "RES 0,L"),
    gb_op(0x86, ARG_BIT_0, ARG_IND_HL, gb_res, 4, "RES 0,(HL)"),
    gb_op(0x87, ARG_BIT_0, ARG_REG_A, gb_res, 2, "RES 0,A"),
    gb_op(0x88, ARG_BIT_1, ARG_REG_B, gb_res, 2, "RES 1,B"),
    gb_op(0x89, ARG_BIT_1, ARG_REG_C, gb_res, 2, "RES 1,C"),
    gb_op(0x8A, ARG_BIT_1, ARG_REG_D, gb_res, 2, "RES 1,D"),
    gb_op(0x8B, ARG_BIT_1, ARG_REG_E, gb_res, 2, "RES 1,E"),
    gb_op(0x8C, ARG_BIT_1, ARG_REG_H, gb_res, 2, "RES 1,H"),
    gb_op(0x8D, ARG_BIT_1, ARG_REG_L, gb_res, 2, "RES 1,L"),
    gb_op(0x8E, ARG_BIT_1, ARG_IND_HL, gb_res, 4, "RES 1,(HL)"),
    gb_op(0x8F, ARG_BIT_1, ARG_REG_A, gb_res, 2, "RES 1,A"),
    gb_op(0x90, ARG_BIT_2, ARG_REG_B, gb_res, 2, "RES 2,B"),
    gb_op(0x91, ARG_BIT_2, ARG_REG_C, gb_res, 2, "RES 2,C"),
    gb_op(0x92, ARG_BIT_2, ARG_REG_D, gb_res, 2, "RES 2,D"),
    gb_op(0x93, ARG_BIT_2, ARG_REG_E, gb_res, 2, "RES 2,E"),
    gb_op(0x94, ARG_BIT_2, ARG_REG_H, gb_res, 2, "RES 2,H"),
    gb_op(0x95, ARG_BIT_2, ARG_REG_L, gb_res, 2, "RES 2,L"),
    gb_op(0x96, ARG_BIT_2, ARG_IND_HL, gb_res, 4, "RES 2,(HL)"),
    gb_op(0x97, ARG_BIT_2, ARG_REG_A, gb_res, 2, "RES 2,A"),
    gb_op(0x98, ARG_BIT_3, ARG_REG_B, gb_res, 2, "RES 3,B"),
    gb_op(0x99, ARG_BIT_3, ARG_REG_C, gb_res, 2, "RES 3,C"),
    gb_op(0x9A, ARG_BIT_3, ARG_REG_D, gb_res, 2, "RES 3,D"),
    gb_op(0x9B, ARG_BIT_3, ARG_REG_E, gb_res, 2, "RES 3,E"),
    gb_op(0x9C, ARG_BIT_3, ARG_REG_H, gb_res, 2, "RES 3,H"),
    gb_op(0x9D, ARG_BIT_3, ARG_REG_L, gb_res, 2, "RES 3,L"),
    gb_op(0x9E, ARG_BIT_3, ARG_IND_HL, gb_res, 4, "RES 3,(HL)"),
    gb_op(0x9F, ARG_BIT_3, ARG_REG_A, gb_res, 2, "RES 3,A"),
    gb_op(0xA0, ARG_BIT_4, ARG_REG_B, gb_res, 2, "RES 4,B"),
    gb_op(0xA1, ARG_BIT_4, ARG_REG_C, gb_res, 2, "RES 4,C"),
    gb_op(0xA2, ARG_BIT_4, ARG_REG_D, gb_res, 2, "RES 4,D"),
    gb_op(0xA3, ARG_BIT_4, ARG_REG_E, gb_res, 2, "RES 4,E"),
    gb_op(0xA4, ARG_BIT_4, ARG_REG_H, gb_res, 2, "RES 4,H"),
    gb_op(0xA5, ARG_BIT_4, ARG_REG_L, gb_res, 2, "RES 4,L"),
    gb_op(0xA6, ARG_BIT_4, ARG_IND_HL, gb_res, 4, "RES 4,(HL)"),
    gb_op(0xA7, ARG_BIT_4, ARG_REG_A, gb_res, 2, "RES 4,A"),
    gb_op(0xA8, ARG_BIT_5, ARG_REG_B, gb_res, 2, "RES 5,B"),
    gb_op(0xA9, ARG_BIT_5, ARG_REG_C, gb_res, 2, "RES 5,C"),

    gb_op(0xAA, ARG_BIT_5, ARG_REG_D, gb_res, 2, "RES 5,D"),
    gb_op(0xAB, ARG_BIT_5, ARG_REG_E, gb_res, 2, "RES 5,E"),
    gb_op(0xAC, ARG_BIT_5, ARG_REG_H, gb_res, 2, "RES 5,H"),
    gb_op(0xAD, ARG_BIT_5, ARG_REG_L, gb_res, 2, "RES 5,L"),
    gb_op(0xAE, ARG_BIT_5, ARG_IND_HL, gb_res, 4, "RES 5,(HL)"),
    gb_op(0xAF, ARG_BIT_5, ARG_REG_A, gb_res, 2, "RES 5,A"),
    gb_op(0xB0, ARG_BIT_6, ARG_REG_B, gb_res, 2, "RES 6,B"),
    gb_op(0xB1, ARG_BIT_6, ARG_REG_C, gb_res, 2, "RES 6,C"),
    gb_op(0xB2, ARG_BIT_6, ARG_REG_D, gb_res, 2, "RES 6,D"),
    gb_op(0xB3, ARG_BIT_6, ARG_REG_E, gb_res, 2, "RES 6,E"),
    gb_op(0xB4, ARG_BIT_6, ARG_REG_H, gb_res, 2, "RES 6,H"),
    gb_op(0xB5, ARG_BIT_6, ARG_REG_L, gb_res, 2, "RES 6,L"),
    gb_op(0xB6, ARG_BIT_6, ARG_IND_HL, gb_res, 4, "RES 6,(HL)"),
    gb_op(0xB7, ARG_BIT_6, ARG_REG_A, gb_res, 2, "RES 6,A"),
    gb_op(0xB8, ARG_BIT_7, ARG_REG_B, gb_res, 2, "RES 7,B"),
    gb_op(0xB9, ARG_BIT_7, ARG_REG_C, gb_res, 2, "RES 7,C"),
    gb_op(0xBA, ARG_BIT_7, ARG_REG_D, gb_res, 2, "RES 7,D"),
    gb_op(0xBB, ARG_BIT_7, ARG_REG_E, gb_res, 2, "RES 7,E"),
    gb_op(0xBC, ARG_BIT_7, ARG_REG_H, gb_res, 2, "RES 7,H"),
    gb_op(0xBD, ARG_BIT_7, ARG_REG_L, gb_res, 2, "RES 7,L"),
    gb_op(0xBE, ARG_BIT_7, ARG_IND_HL, gb_res, 4, "RES 7,(HL)"),
    gb_op(0xBF, ARG_BIT_7, ARG_REG_A, gb_res, 2, "RES 7,A"),

    gb_op(0xC0, ARG_BIT_0, ARG_REG_B, gb_set, 2, "SET 0,B"),
    gb_op(0xC1, ARG_BIT_0, ARG_REG_C, gb_set, 2, "SET 0,C"),
    gb_op(0xC2, ARG_BIT_0, ARG_REG_D, gb_set, 2, "SET 0,D"),
    gb_op(0xC3, ARG_BIT_0, ARG_REG_E, gb_set, 2, "SET 0,E"),
    gb_op(0xC4, ARG_BIT_0, ARG_REG_H, gb_set, 2, "SET 0,H"),
    gb_op(0xC5, ARG_BIT_0, ARG_REG_L, gb_set, 2, "SET 0,L"),
    gb_op(0xC6, ARG_BIT_0, ARG_IND_HL, gb_set, 4, "SET 0,(HL)"),
    gb_op(0xC7, ARG_BIT_0, ARG_REG_A, gb_set, 2, "SET 0,A"),
    gb_op(0xC8, ARG_BIT_1, ARG_REG_B, gb_set, 2, "SET 1,B"),
    gb_op(0xC9, ARG_BIT_1, ARG_REG_C, gb_set, 2, "SET 1,C"),
    gb_op(0xCA, ARG_BIT_1, ARG_REG_D, gb_set, 2, "SET 1,D"),
    gb_op(0xCB, ARG_BIT_1, ARG_REG_E, gb_set, 2, "SET 1,E"),
    gb_op(0xCC, ARG_BIT_1, ARG_REG_H, gb_set, 2, "SET 1,H"),
    gb_op(0xCD, ARG_BIT_1, ARG_REG_L, gb_set, 2, "SET 1,L"),
    gb_op(0xCE, ARG_BIT_1, ARG_IND_HL, gb_set, 4, "SET 1,(HL)"),
    gb_op(0xCF, ARG_BIT_1, ARG_REG_A, gb_set, 2, "SET 1,A"),
    gb_op(0xD0, ARG_BIT_2, ARG_REG_B, gb_set, 2, "SET 2,B"),
    gb_op(0xD1, ARG_BIT_2, ARG_REG_C, gb_set, 2, "SET 2,C"),
    gb_op(0xD2, ARG_BIT_2, ARG_REG_D, gb_set, 2, "SET 2,D"),
    gb_op(0xD3, ARG_BIT_2, ARG_REG_E, gb_set, 2, "SET 2,E"),
    gb_op(0xD4, ARG_BIT_2, ARG_REG_H, gb_set, 2, "SET 2,H"),
    gb_op(0xD5, ARG_BIT_2, ARG_REG_L, gb_set, 2, "SET 2,L"),
    gb_op(0xD6, ARG_BIT_2, ARG_IND_HL, gb_set, 4, "SET 2,(HL)"),
    gb_op(0xD7, ARG_BIT_2, ARG_REG_A, gb_set, 2, "SET 2,A"),
    gb_op(0xD8, ARG_BIT_3, ARG_REG_B, gb_set, 2, "SET 3,B"),
    gb_op(0xD9, ARG_BIT_3, ARG_REG_C, gb_set, 2, "SET 3,C"),
    gb_op(0xDA, ARG_BIT_3, ARG_REG_D, gb_set, 2, "SET 3,D"),
    gb_op(0xDB, ARG_BIT_3, ARG_REG_E, gb_set, 2, "SET 3,E"),
    gb_op(0xDC, ARG_BIT_3, ARG_REG_H, gb_set, 2, "SET 3,H"),
    gb_op(0xDD, ARG_BIT_3, ARG_REG_L, gb_set, 2, "SET 3,L"),
    gb_op(0xDE, ARG_BIT_3, ARG_IND_HL, gb_set, 4, "SET 3,(HL)"),
    gb_op(0xDF, ARG_BIT_3, ARG_REG_A, gb_set, 2, "SET 3,A"),
    gb_op(0xE0, ARG_BIT_4, ARG_REG_B, gb_set, 2, "SET 4,B"),
    gb_op(0xE1, ARG_BIT_4, ARG_REG_C, gb_set, 2, "SET 4,C"),
    gb_op(0xE2, ARG_BIT_4, ARG_REG_D, gb_set, 2, "SET 4,D"),
    gb_op(0xE3, ARG_BIT_4, ARG_REG_E, gb_set, 2, "SET 4,E"),
    gb_op(0xE4, ARG_BIT_4, ARG_REG_H, gb_set, 2, "SET 4,H"),
    gb_op(0xE5, ARG_BIT_4, ARG_REG_L, gb_set, 2, "SET 4,L"),
    gb_op(0xE6, ARG_BIT_4, ARG_IND_HL, gb_set, 4, "SET 4,(HL)"),
    gb_op(0xE7, ARG_BIT_4, ARG_REG_A, gb_set, 2, "SET 4,A"),
    gb_op(0xE8, ARG_BIT_5, ARG_REG_B, gb_set, 2, "SET 5,B"),
    gb_op(0xE9, ARG_BIT_5, ARG_REG_C, gb_set, 2, "SET 5,C"),
    gb_op(0xEA, ARG_BIT_5, ARG_REG_D, gb_set, 2, "SET 5,D"),
    gb_op(0xEB, ARG_BIT_5, ARG_REG_E, gb_set, 2, "SET 5,E"),
    gb_op(0xEC, ARG_BIT_5, ARG_REG_H, gb_set, 2, "SET 5,H"),
    gb_op(0xED, ARG_BIT_5, ARG_REG_L, gb_set, 2, "SET 5,L"),
    gb_op(0xEE, ARG_BIT_5, ARG_IND_HL, gb_set, 4, "SET 5,(HL)"),
    gb_op(0xEF, ARG_BIT_5, ARG_REG_A, gb_set, 2, "SET 5,A"),
    gb_op(0xF0, ARG_BIT_6, ARG_REG_B, gb_set, 2, "SET 6,B"),
    gb_op(0xF1, ARG_BIT_6, ARG_REG_C, gb_set, 2, "SET 6,C"),
    gb_op(0xF2, ARG_BIT_6, ARG_REG_D, gb_set, 2, "SET 6,D"),
    gb_op(0xF3, ARG_BIT_6, ARG_REG_E, gb_set, 2, "SET 6,E"),
    gb_op(0xF4, ARG_BIT_6, ARG_REG_H, gb_set, 2, "SET 6,H"),
    gb_op(0xF5, ARG_BIT_6, ARG_REG_L, gb_set, 2, "SET 6,L"),
    gb_op(0xF6, ARG_BIT_6, ARG_IND_HL, gb_set, 4, "SET 6,(HL)"),
    gb_op(0xF7, ARG_BIT_6, ARG_REG_A, gb_set, 2, "SET 6,A"),
    gb_op(0xF8, ARG_BIT_7, ARG_REG_B, gb_set, 2, "SET 7,B"),
    gb_op(0xF9, ARG_BIT_7, ARG_REG_C, gb_set, 2, "SET 7,C"),
    gb_op(0xFA, ARG_BIT_7, ARG_REG_D, gb_set, 2, "SET 7,D"),
    gb_op(0xFB, ARG_BIT_7, ARG_REG_E, gb_set, 2, "SET 7,E"),
    gb_op(0xFC, ARG_BIT_7, ARG_REG_H, gb_set, 2, "SET 7,H"),
    gb_op(0xFD, ARG_BIT_7, ARG_REG_L, gb_set, 2, "SET 7,L"),
    gb_op(0xFE, ARG_BIT_7, ARG_IND_HL, gb_set, 4, "SET 7,(HL)"),
    gb_op(0xFF, ARG_BIT_7, ARG_REG_A, gb_set, 2, "SET 7,A"),
};
