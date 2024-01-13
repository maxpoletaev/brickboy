#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "shared.h"
#include "cpu.h"
#include "bus.h"

static const uint16_t reset_addr[8] = {
    0x00, 0x08, 0x10, 0x18,
    0x20, 0x28, 0x30, 0x38,
};

void
cpu_reset(CPU *cpu)
{
    cpu->af = 0x01B0;
    cpu->bc = 0x0013;
    cpu->de = 0x00D8;
    cpu->hl = 0x014D;
    cpu->sp = 0xFFFE;
    cpu->pc = 0x0100; // skip the boot rom for now
    cpu->ime = 0;
    cpu->remaining = 0;
    cpu->cycle = 0;
    cpu->ime_delay = -1;
}

static uint16_t
cpu_get_operand(CPU *cpu, Bus *bus, Operand src)
{
    uint16_t value = 0;
    uint16_t addr = 0;

    switch (src) {
    case ARG_NONE:
        PANIC("invalid source operand: %d", src);
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
        value = reset_addr[src - ARG_RST_0];
        break;
    case ARG_IMM16:
        value = bus_read16(bus, cpu->pc);
        cpu->pc += 2;
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
        value = bus_read(bus, addr);
        break;
    case ARG_IND_BC:
        value = bus_read(bus, cpu->bc);
        break;
    case ARG_IND_DE:
        value = bus_read(bus, cpu->de);
        break;
    case ARG_IND_HL:
        value = bus_read(bus, cpu->hl);
        break;
    case ARG_IND_HLI:
        value = bus_read(bus, cpu->hl++);
        break;
    case ARG_IND_HLD:
        value = bus_read(bus, cpu->hl--);
        break;
    case ARG_IMM8:
        value = bus_read(bus, cpu->pc++);
        break;
    case ARG_IND_IMM8:
        addr = 0xFF00 + bus_read(bus, cpu->pc++);
        value = bus_read(bus, addr);
        break;
    case ARG_IND_IMM16:
        addr = bus_read16(bus, cpu->pc);
        value = bus_read(bus, addr);
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
cpu_set_operand(CPU *cpu, Bus *bus, Operand target, uint16_t value16)
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
        PANIC("invalid target operand: %d", target);
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
            bus_write(bus, addr, value);
        break;
    case ARG_IND_BC:
        bus_write(bus, cpu->bc, value);
        break;
    case ARG_IND_DE:
        bus_write(bus, cpu->de, value);
        break;
    case ARG_IND_HL:
        bus_write(bus, cpu->hl, value);
        break;
    case ARG_IND_HLI:
        bus_write(bus, cpu->hl++, value);
        break;
    case ARG_IND_HLD:
        bus_write(bus, cpu->hl--, value);
        break;
    case ARG_IND_IMM8:
        addr = 0xFF00 + bus_read(bus, cpu->pc++);
            bus_write(bus, addr, value);
        break;
    case ARG_IND_IMM16:
        addr = bus_read16(bus, cpu->pc);
            bus_write(bus, addr, value);
        cpu->pc += 2;
        break;
    }
}

static uint8_t
cpu_getbit(CPU *cpu, Bus *bus, Operand arg, uint8_t bit)
{
    uint8_t value = (uint8_t) cpu_get_operand(cpu, bus, arg);
    return (value >> bit) & 1;
}

static void
cpu_setbit(CPU *cpu, Bus *bus, Operand arg, uint8_t bit, uint8_t value)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, arg);

    assert(bit < 8);
    v &= ~(1 << bit); // clear bit first
    v |= (value & 1) << bit; // OR in the new value

    cpu_set_operand(cpu, bus, arg, v);
}

static void
cpu_push(CPU *cpu, Bus *bus, uint16_t value)
{
    cpu->sp -= 2;
    bus_write16(bus, cpu->sp, value);
}

static uint16_t
cpu_pop(CPU *cpu, Bus *bus)
{
    uint16_t value = bus_read16(bus, cpu->sp);
    cpu->sp += 2;
    return value;
}

const Instruction *
cpu_decode(Bus *bus, uint16_t pc)
{
    uint8_t opcode = bus_read(bus, pc);
    const Instruction *op = &opcodes[opcode];

    // Prefixed instructions
    if (opcode == 0xCB) {
        opcode = bus_read(bus, pc + 1);
        op = &cb_opcodes[opcode];
    }

    return op;
}

void
cpu_interrupt(CPU *cpu, Bus *bus, Interrupt interrupt)
{
    cpu->halted = false;
    if (cpu->ime == 0)
        return;

    cpu->ime = 0; // disable interrupts
    cpu_push(cpu, bus, cpu->pc);

    switch (interrupt) {
    case INTERRUPT_VBLANK:
        cpu->pc = 0x0040;
        break;
    case INTERRUPT_LCDSTAT:
        cpu->pc = 0x0048;
        break;
    case INTERRUPT_TIMER:
        cpu->pc = 0x0050;
        break;
    case INTERRUPT_SERIAL:
        cpu->pc = 0x0058;
        break;
    case INTERRUPT_JOYPAD:
        cpu->pc = 0x0060;
        break;
    }
}

void
cpu_step(CPU *cpu, Bus *bus)
{

    if (cpu->halted)
        return;

    cpu->cycle++;

    // Currently executing an instruction.
    if (cpu->remaining > 0) {
        cpu->remaining--;
        return;
    }

    uint8_t opcode = bus_read(bus, cpu->pc++);
    const Instruction *op = &opcodes[opcode];

    // Prefixed instructions
    if (opcode == 0xCB) {
        opcode = bus_read(bus, cpu->pc++);
        op = &cb_opcodes[opcode];
    }

    if (op->handler == NULL)
        PANIC("invalid opcode: 0x%02X", opcode);

    cpu->remaining = op->cycles - 1;
    op->handler(cpu, bus, op);

    // EI takes effect one instruction later
    if (cpu->ime_delay != -1) {
        cpu->ime = cpu->ime_delay;
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
nop(CPU *cpu, Bus *bus, const Instruction *op)
{
    UNUSED(cpu);
    UNUSED(bus);
    UNUSED(op);
}

static void
halt(CPU *cpu, Bus *bus, const Instruction *op)
{
    UNUSED(bus);
    UNUSED(op);

    cpu->halted = true;
}

/* DAA
 * Flags: Z - 0 C
 * Decimal adjust register A. */
static void
daa(CPU *cpu, Bus *bus, const Instruction *op)
{
    UNUSED(bus);
    UNUSED(op);

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
cpl(CPU *cpu, Bus *bus, const Instruction *op)
{
    UNUSED(bus);
    UNUSED(op);

    cpu->a = ~cpu->a;
    cpu->flags.negative = 1;
    cpu->flags.half_carry = 1;
}

/* SCF
 * Flags: - 0 0 1
 * Set carry flag. */
static void
scf(CPU *cpu, Bus *bus, const Instruction *op)
{
    UNUSED(bus);
    UNUSED(op);

    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = 1;
}

/* CCF
 * Flags: - 0 0 C
 * Complement carry flag. */
static void
ccf(CPU *cpu, Bus *bus, const Instruction *op)
{
    UNUSED(bus);
    UNUSED(op);

    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = !cpu->flags.carry;
}

/* LD r8,r8
 * Flags: - - - -
 * Load 8-bit register or memory location into another 8-bit register or memory location. */
static void
ld8(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg2);
    cpu_set_operand(cpu, bus, op->arg1, v);
}

/* LD r16,r16
 * Flags: - - - -
 * Load 16-bit data into 16-bit register. */
static void
ld16(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint16_t v = cpu_get_operand(cpu, bus, op->arg2);
    cpu_set_operand(cpu, bus, op->arg1, v);
}

/* INC r8
 * Flags: Z 0 H -
 * Increment 8-bit register or memory location. */
static void
inc8(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);

    uint8_t half_sum = (v&0xF) + 1;
    uint8_t r = v + 1;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = half_sum > 0xF;

    cpu_set_operand(cpu, bus, op->arg1, r);
}

/* INC r16
 * Flags: - - - -
 * Increment 16-bit register. */
static void
inc16(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint16_t v = cpu_get_operand(cpu, bus, op->arg1);
    uint16_t r = v + 1;

    cpu_set_operand(cpu, bus, op->arg1, r);
}

/* DEC r8
 * Flags: Z 1 H -
 * Decrement 8-bit register or memory location. */
static void
dec8(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t r = v - 1;

    uint8_t half_sum = (v&0xF) - 1;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 1;
    cpu->flags.half_carry = half_sum > 0xF;

    cpu_set_operand(cpu, bus, op->arg1, r);
}

/* DEC r16
 * Flags: - - - -
 * Decrement 16-bit register. */
static void
dec16(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint16_t v = cpu_get_operand(cpu, bus, op->arg1);
    uint16_t r = v - 1;

    cpu_set_operand(cpu, bus, op->arg1, r);
}

/* ADD A,r8
 * Flags: Z 0 H C
 * Add 8-bit register or memory location to A. */
static void
add_a(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
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
add_hl(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint16_t v = cpu_get_operand(cpu, bus, op->arg1);
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
add_sp(CPU *cpu, Bus *bus, const Instruction *op)
{
    int8_t v = (int8_t) cpu_get_operand(cpu, bus, op->arg1);
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
adc8(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
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
sub8(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
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
sbc8(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
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
and8(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
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
jp16(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint16_t addr = cpu_get_operand(cpu, bus, op->arg1);
    cpu->pc = addr;
}

/* JP F,a16
 * Flags: - - - -
 * Jump to 16-bit address provided by immediate operand or register if F flag is set. */
static void
jp16_if(CPU *cpu, Bus *bus, const Instruction *op)
{
    bool flag = (bool) cpu_get_operand(cpu, bus, op->arg1);
    uint16_t addr = cpu_get_operand(cpu, bus, op->arg2);

    if (flag) {
        cpu->pc = addr;
        cpu->remaining += 1;
    }
}

/* JP !F,a16
 * Flags: - - - -
 * Jump to 16-bit address provided by immediate operand or register if F flag is not set. */
static void
jp16_ifn(CPU *cpu, Bus *bus, const Instruction *op)
{
    bool flag = (bool) cpu_get_operand(cpu, bus, op->arg1);
    uint16_t addr = cpu_get_operand(cpu, bus, op->arg2);

    if (!flag) {
        cpu->pc = addr;
        cpu->remaining += 1;
    }
}

/* JR s8
 * Flags: - - - -
 * Jump to 8-bit signed offset. */
static void
jr8(CPU *cpu, Bus *bus, const Instruction *op)
{
    int8_t offset = (int8_t) cpu_get_operand(cpu, bus, op->arg1);
    cpu->pc += offset;
}

/* JR F,s8
 * Flags: - - - -
 * Jump to 8-bit signed offset if F flag is set. */
static void
jr8_if(CPU *cpu, Bus *bus, const Instruction *op)
{
    bool flag = (bool) cpu_get_operand(cpu, bus, op->arg1);
    int8_t offset = (int8_t) cpu_get_operand(cpu, bus, op->arg2);

    if (flag) {
        cpu->pc += offset;
        cpu->remaining += 1;
    }
}

/* JR !F,s8
 * Flags: - - - -
 * Jump to 8-bit signed offset if F flag is not set. */
static void
jr8_ifn(CPU *cpu, Bus *bus, const Instruction *op)
{
    bool flag = (bool) cpu_get_operand(cpu, bus, op->arg1);
    int8_t offset = (int8_t) cpu_get_operand(cpu, bus, op->arg2);

    if (!flag) {
        cpu->pc += offset;
        cpu->remaining += 1;
    }
}

/* XOR r8
 * Flags: Z 0 0 0
 * XOR 8-bit register or memory location with A. */
static void
xor8(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
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
or8(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
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
cp8(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
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
push16(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint16_t v = cpu_get_operand(cpu, bus, op->arg1);
    cpu_push(cpu, bus, v);
}

/* POP r16
 * Flags: - - - -
 * Pop 16-bit register off stack. */
static void
pop16(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint16_t v = cpu_pop(cpu, bus);

    if (op->arg1 == ARG_REG_AF)
        v &= 0xFFF0; // lower 4 bits of F are always zero

    cpu_set_operand(cpu, bus, op->arg1, v);
}

/* CALL a16
 * Flags: - - - -
 * Call 16-bit address provided by immediate operand or register. */
static void
call(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint16_t addr = cpu_get_operand(cpu, bus, op->arg1);
    cpu_push(cpu, bus, cpu->pc);
    cpu->pc = addr;
}

/* CALL F,a16
 * Flags: - - - -
 * Call 16-bit address provided by immediate operand or register if F flag is set. */
static void
call_if(CPU *cpu, Bus *bus, const Instruction *op)
{
    bool flag = (bool) cpu_get_operand(cpu, bus, op->arg1);
    uint16_t addr = cpu_get_operand(cpu, bus, op->arg2);

    if (flag) {
        cpu_push(cpu, bus, cpu->pc);
        cpu->remaining += 3;
        cpu->pc = addr;
    }
}

/* CALL !F,a16
 * Flags: - - - -
 * Call 16-bit address provided by immediate operand or register if F flag is not set. */
static void
call_ifn(CPU *cpu, Bus *bus, const Instruction *op)
{
    bool flag = (bool) cpu_get_operand(cpu, bus, op->arg1);
    uint16_t addr = cpu_get_operand(cpu, bus, op->arg2);

    if (!flag) {
        cpu_push(cpu, bus, cpu->pc);
        cpu->remaining += 3;
        cpu->pc = addr;
    }
}

/* RET
 * Flags: - - - -
 * Return from subroutine. */
static void
ret(CPU *cpu, Bus *bus, const Instruction *op)
{
    UNUSED(bus);
    UNUSED(op);

    cpu->pc = cpu_pop(cpu, bus);
}

/* RET F
 * Flags: - - - -
 * Return from subroutine if F flag is set. */
static void
ret_if(CPU *cpu, Bus *bus, const Instruction *op)
{
    bool flag = (bool) cpu_get_operand(cpu, bus, op->arg1);

    if (flag) {
        cpu->pc = cpu_pop(cpu, bus);
        cpu->remaining += 3;
    }
}

/* RET !F
 * Flags: - - - -
 * Return from subroutine if F flag is not set. */
static void
ret_ifn(CPU *cpu, Bus *bus, const Instruction *op)
{
    bool flag = (bool) cpu_get_operand(cpu, bus, op->arg1);

    if (!flag) {
        cpu->pc = cpu_pop(cpu, bus);
        cpu->remaining += 3;
    }
}

/* RETI
 * Flags: - - - -
 * Return from subroutine and enable interrupts. */
static void
reti(CPU *cpu, Bus *bus, const Instruction *op)
{
    UNUSED(bus);
    UNUSED(op);

    cpu->pc = cpu_pop(cpu, bus);
    cpu->ime = 1; // looks like not delayed unlike EI
}

/* RST n
 * Flags: - - - -
 * Push present address onto stack and jump to address $0000 + n,
 * where n is one of $00, $08, $10, $18, $20, $28, $30, $38. */
static void
rst(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint16_t addr = cpu_get_operand(cpu, bus, op->arg1);
    cpu_push(cpu, bus, cpu->pc);
    cpu->pc = addr;
}

/* RLCA
 * RLC r8
 * Flags: Z 0 0 C
 * Rotate register left. */
static void
rlc(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t r = (uint8_t) (v << 1) | (v >> 7);

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = (v >> 7) & 1;

    cpu_set_operand(cpu, bus, op->arg1, r);
}

/* RLA
 * Flags: 0 0 0 C
 * Rotate register A left through carry flag. */
static void
rla(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t r = (uint8_t) ((v << 1) | cpu->flags.carry);

    cpu->flags.zero = 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = (v >> 7) & 1;

    cpu_set_operand(cpu, bus, op->arg1, r);
}

/* RL r8
 * Flags: Z 0 0 C
 * Rotate register left through carry flag. */
static void
rl(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t r = (uint8_t) ((v << 1) | cpu->flags.carry);

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = (v >> 7) & 1;

    cpu_set_operand(cpu, bus, op->arg1, r);
}

/* RRCA
 * Flags: 0 0 0 C
 * Rotate register right. */
static void
rrca(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t r = (uint8_t) ((v >> 1) | (v << 7));

    cpu->flags.zero = 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = v & 1;

    cpu_set_operand(cpu, bus, op->arg1, r);
}

/* RRC r8
 * Flags: Z 0 0 C
 * Rotate register right. */
static void
rrc(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t r = (uint8_t) ((v >> 1) | (v << 7));

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = v & 1;

    cpu_set_operand(cpu, bus, op->arg1, r);
}

/* RRA
 * Flags: 0 0 0 C
 * Rotate register A right through carry flag. */
static void
rra(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t r = (uint8_t) ((v >> 1) | (cpu->flags.carry << 7));

    cpu->flags.zero = 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = v & 1;

    cpu_set_operand(cpu, bus, op->arg1, r);
}


/* RR r8
 * Flags: Z 0 0 C
 * Rotate register right through carry flag. */
static void
rr(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t r = (uint8_t) ((v >> 1) | (cpu->flags.carry << 7));

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = v & 1;

    cpu_set_operand(cpu, bus, op->arg1, r);
}

/* SLA n
 * Flags: Z 0 0 C
 * Shift register left into carry. */
static void
sla(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t r = (uint8_t) (v << 1);

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = (v >> 7) & 1;

    cpu_set_operand(cpu, bus, op->arg1, r);
}

/* SRA n
 * Flags: Z 0 0 C
 * Shift register right into carry. MSB is unchanged. */
static void
sra(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t r = (v >> 1) | (v & 0x80);

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = v & 1;

    cpu_set_operand(cpu, bus, op->arg1, r);
}

/* SRL n
 * Flags: Z 0 0 C
 * Shift register right into carry. MSB is set to 0. C flag is old LSB. */
static void
srl(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t r = v >> 1;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = v & 1;

    cpu_set_operand(cpu, bus, op->arg1, r);
}

/* DI
 * Flags: - - - -
 * Disable interrupts. */
static void
di(CPU *cpu, Bus *bus, const Instruction *op)
{
    UNUSED(bus);
    UNUSED(op);

    cpu->ime = 0;
}

/* EI
 * Flags: - - - -
 * Enable interrupts. */
static void
ei(CPU *cpu, Bus *bus, const Instruction *op)
{
    UNUSED(bus);
    UNUSED(op);

    cpu->ime_delay = 1;
}

/* SWAP n
 * Flags: Z 0 0 0
 * Swap upper and lower nibbles of register. */
static void
swap(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t l = (uint8_t) ((v & 0x0F) << 4);
    uint8_t h = (uint8_t) ((v & 0xF0) >> 4);
    uint8_t r = l | h;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = 0;

    cpu_set_operand(cpu, bus, op->arg1, r);
}

/* BIT b,r8
 * Flags: Z 0 1 -
 * Test bit b in 8-bit register or memory location. */
static void
bit(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t bit = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t v = cpu_getbit(cpu, bus, op->arg2, bit);

    cpu->flags.zero = v == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 1;
}

/* SET b,r8
 * Flags: - - - -
 * Set bit b in 8-bit register or memory location. */
static void
set(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t bit = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    cpu_setbit(cpu, bus, op->arg2, bit, 1);
}

/* RES b,r8
 * Flags: - - - -
 * Reset bit b in 8-bit register or memory location. */
static void
res(CPU *cpu, Bus *bus, const Instruction *op)
{
    uint8_t bit = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    cpu_setbit(cpu, bus, op->arg2, bit, 0);
}

/* ----------------------------------------------------------------------------
 * Opcode Tables
 * -------------------------------------------------------------------------- */

#define OPCODE(op, arg1, arg2, handler, cost, text) \
    [op] = {op, cost, arg1, arg2, handler, text}

const Instruction opcodes[256] = {
    OPCODE(0x00, ARG_NONE, ARG_NONE, nop, 1, "NOP"),
    OPCODE(0xF3, ARG_NONE, ARG_NONE, di, 1, "DI"),
    OPCODE(0xFB, ARG_NONE, ARG_NONE, ei, 1, "EI"),
    OPCODE(0x27, ARG_NONE, ARG_NONE, daa, 1, "DAA"),
    OPCODE(0x2F, ARG_NONE, ARG_NONE, cpl, 1, "CPL"),
    OPCODE(0x37, ARG_NONE, ARG_NONE, scf, 1, "SCF"),
    OPCODE(0x3F, ARG_NONE, ARG_NONE, ccf, 1, "CCF"),
    OPCODE(0x76, ARG_NONE, ARG_NONE, halt, 1, "HALT"),

    OPCODE(0x04, ARG_REG_B, ARG_NONE, inc8, 1, "INC B"),
    OPCODE(0x0C, ARG_REG_C, ARG_NONE, inc8, 1, "INC C"),
    OPCODE(0x14, ARG_REG_D, ARG_NONE, inc8, 1, "INC D"),
    OPCODE(0x1C, ARG_REG_E, ARG_NONE, inc8, 1, "INC E"),
    OPCODE(0x24, ARG_REG_H, ARG_NONE, inc8, 1, "INC H"),
    OPCODE(0x2C, ARG_REG_L, ARG_NONE, inc8, 1, "INC L"),
    OPCODE(0x3C, ARG_REG_A, ARG_NONE, inc8, 1, "INC A"),
    OPCODE(0x34, ARG_IND_HL, ARG_NONE, inc8, 3, "INC (HL)"),

    OPCODE(0x03, ARG_REG_BC, ARG_NONE, inc16, 2, "INC BC"),
    OPCODE(0x13, ARG_REG_DE, ARG_NONE, inc16, 2, "INC DE"),
    OPCODE(0x23, ARG_REG_HL, ARG_NONE, inc16, 2, "INC HL"),
    OPCODE(0x33, ARG_REG_SP, ARG_NONE, inc16, 2, "INC SP"),

    OPCODE(0x05, ARG_REG_B, ARG_NONE, dec8, 1, "DEC B"),
    OPCODE(0x0D, ARG_REG_C, ARG_NONE, dec8, 1, "DEC C"),
    OPCODE(0x15, ARG_REG_D, ARG_NONE, dec8, 1, "DEC D"),
    OPCODE(0x1D, ARG_REG_E, ARG_NONE, dec8, 1, "DEC E"),
    OPCODE(0x25, ARG_REG_H, ARG_NONE, dec8, 1, "DEC H"),
    OPCODE(0x2D, ARG_REG_L, ARG_NONE, dec8, 1, "DEC L"),
    OPCODE(0x3D, ARG_REG_A, ARG_NONE, dec8, 1, "DEC A"),
    OPCODE(0x35, ARG_IND_HL, ARG_NONE, dec8, 3, "DEC (HL)"),

    OPCODE(0x0B, ARG_REG_BC, ARG_NONE, dec16, 2, "DEC BC"),
    OPCODE(0x1B, ARG_REG_DE, ARG_NONE, dec16, 2, "DEC DE"),
    OPCODE(0x2B, ARG_REG_HL, ARG_NONE, dec16, 2, "DEC HL"),
    OPCODE(0x3B, ARG_REG_SP, ARG_NONE, dec16, 2, "DEC SP"),

    OPCODE(0x02, ARG_IND_BC, ARG_REG_A, ld8, 2, "LD (BC),A"),
    OPCODE(0x0A, ARG_REG_A, ARG_IND_BC, ld8, 2, "LD A,(BC)"),
    OPCODE(0x12, ARG_IND_DE, ARG_REG_A, ld8, 2, "LD (DE),A"),
    OPCODE(0x1A, ARG_REG_A, ARG_IND_DE, ld8, 2, "LD A,(DE)"),
    OPCODE(0x22, ARG_IND_HLI, ARG_REG_A, ld8, 2, "LD (HL+),A"),
    OPCODE(0x2A, ARG_REG_A, ARG_IND_HLI, ld8, 2, "LD A,(HL+)"),
    OPCODE(0x32, ARG_IND_HLD, ARG_REG_A, ld8, 2, "LD (HL-),A"),
    OPCODE(0x3A, ARG_REG_A, ARG_IND_HLD, ld8, 2, "LD A,(HL-)"),
    OPCODE(0x40, ARG_REG_B, ARG_REG_B, ld8, 1, "LD B,B"),
    OPCODE(0x41, ARG_REG_B, ARG_REG_C, ld8, 1, "LD B,C"),
    OPCODE(0x42, ARG_REG_B, ARG_REG_D, ld8, 1, "LD B,D"),
    OPCODE(0x43, ARG_REG_B, ARG_REG_E, ld8, 1, "LD B,E"),
    OPCODE(0x44, ARG_REG_B, ARG_REG_H, ld8, 1, "LD B,H"),
    OPCODE(0x45, ARG_REG_B, ARG_REG_L, ld8, 1, "LD B,L"),
    OPCODE(0x46, ARG_REG_B, ARG_IND_HL, ld8, 2, "LD B,(HL)"),
    OPCODE(0x47, ARG_REG_B, ARG_REG_A, ld8, 1, "LD B,A"),
    OPCODE(0x48, ARG_REG_C, ARG_REG_B, ld8, 1, "LD C,B"),
    OPCODE(0x49, ARG_REG_C, ARG_REG_C, ld8, 1, "LD C,C"),
    OPCODE(0x4A, ARG_REG_C, ARG_REG_D, ld8, 1, "LD C,D"),
    OPCODE(0x4B, ARG_REG_C, ARG_REG_E, ld8, 1, "LD C,E"),
    OPCODE(0x4C, ARG_REG_C, ARG_REG_H, ld8, 1, "LD C,H"),
    OPCODE(0x4D, ARG_REG_C, ARG_REG_L, ld8, 1, "LD C,L"),
    OPCODE(0x4E, ARG_REG_C, ARG_IND_HL, ld8, 2, "LD C,(HL)"),
    OPCODE(0x4F, ARG_REG_C, ARG_REG_A, ld8, 1, "LD C,A"),
    OPCODE(0x50, ARG_REG_D, ARG_REG_B, ld8, 1, "LD D,B"),
    OPCODE(0x51, ARG_REG_D, ARG_REG_C, ld8, 1, "LD D,C"),
    OPCODE(0x52, ARG_REG_D, ARG_REG_D, ld8, 1, "LD D,D"),
    OPCODE(0x53, ARG_REG_D, ARG_REG_E, ld8, 1, "LD D,E"),
    OPCODE(0x54, ARG_REG_D, ARG_REG_H, ld8, 1, "LD D,H"),
    OPCODE(0x55, ARG_REG_D, ARG_REG_L, ld8, 1, "LD D,L"),
    OPCODE(0x56, ARG_REG_D, ARG_IND_HL, ld8, 2, "LD D,(HL)"),
    OPCODE(0x57, ARG_REG_D, ARG_REG_A, ld8, 1, "LD D,A"),
    OPCODE(0x58, ARG_REG_E, ARG_REG_B, ld8, 1, "LD E,B"),
    OPCODE(0x59, ARG_REG_E, ARG_REG_C, ld8, 1, "LD E,C"),
    OPCODE(0x5A, ARG_REG_E, ARG_REG_D, ld8, 1, "LD E,D"),
    OPCODE(0x5B, ARG_REG_E, ARG_REG_E, ld8, 1, "LD E,E"),
    OPCODE(0x5C, ARG_REG_E, ARG_REG_H, ld8, 1, "LD E,H"),
    OPCODE(0x5D, ARG_REG_E, ARG_REG_L, ld8, 1, "LD E,L"),
    OPCODE(0x5E, ARG_REG_E, ARG_IND_HL, ld8, 2, "LD E,(HL)"),
    OPCODE(0x5F, ARG_REG_E, ARG_REG_A, ld8, 1, "LD E,A"),
    OPCODE(0x60, ARG_REG_H, ARG_REG_B, ld8, 1, "LD H,B"),
    OPCODE(0x61, ARG_REG_H, ARG_REG_C, ld8, 1, "LD H,C"),
    OPCODE(0x62, ARG_REG_H, ARG_REG_D, ld8, 1, "LD H,D"),
    OPCODE(0x63, ARG_REG_H, ARG_REG_E, ld8, 1, "LD H,E"),
    OPCODE(0x64, ARG_REG_H, ARG_REG_H, ld8, 1, "LD H,H"),
    OPCODE(0x65, ARG_REG_H, ARG_REG_L, ld8, 1, "LD H,L"),
    OPCODE(0x66, ARG_REG_H, ARG_IND_HL, ld8, 2, "LD H,(HL)"),
    OPCODE(0x67, ARG_REG_H, ARG_REG_A, ld8, 1, "LD H,A"),
    OPCODE(0x68, ARG_REG_L, ARG_REG_B, ld8, 1, "LD L,B"),
    OPCODE(0x69, ARG_REG_L, ARG_REG_C, ld8, 1, "LD L,C"),
    OPCODE(0x6A, ARG_REG_L, ARG_REG_D, ld8, 1, "LD L,D"),
    OPCODE(0x6B, ARG_REG_L, ARG_REG_E, ld8, 1, "LD L,E"),
    OPCODE(0x6C, ARG_REG_L, ARG_REG_H, ld8, 1, "LD L,H"),
    OPCODE(0x6D, ARG_REG_L, ARG_REG_L, ld8, 1, "LD L,L"),
    OPCODE(0x6E, ARG_REG_L, ARG_IND_HL, ld8, 2, "LD L,(HL)"),
    OPCODE(0x6F, ARG_REG_L, ARG_REG_A, ld8, 1, "LD L,A"),
    OPCODE(0x70, ARG_IND_HL, ARG_REG_B, ld8, 2, "LD (HL),B"),
    OPCODE(0x71, ARG_IND_HL, ARG_REG_C, ld8, 2, "LD (HL),C"),
    OPCODE(0x72, ARG_IND_HL, ARG_REG_D, ld8, 2, "LD (HL),D"),
    OPCODE(0x73, ARG_IND_HL, ARG_REG_E, ld8, 2, "LD (HL),E"),
    OPCODE(0x74, ARG_IND_HL, ARG_REG_H, ld8, 2, "LD (HL),H"),
    OPCODE(0x75, ARG_IND_HL, ARG_REG_L, ld8, 2, "LD (HL),L"),
    OPCODE(0x77, ARG_IND_HL, ARG_REG_A, ld8, 2, "LD (HL),A"),
    OPCODE(0x78, ARG_REG_A, ARG_REG_B, ld8, 1, "LD A,B"),
    OPCODE(0x79, ARG_REG_A, ARG_REG_C, ld8, 1, "LD A,C"),
    OPCODE(0x7A, ARG_REG_A, ARG_REG_D, ld8, 1, "LD A,D"),
    OPCODE(0x7B, ARG_REG_A, ARG_REG_E, ld8, 1, "LD A,E"),
    OPCODE(0x7C, ARG_REG_A, ARG_REG_H, ld8, 1, "LD A,H"),
    OPCODE(0x7D, ARG_REG_A, ARG_REG_L, ld8, 1, "LD A,L"),
    OPCODE(0x7E, ARG_REG_A, ARG_IND_HL, ld8, 2, "LD A,(HL)"),
    OPCODE(0x7F, ARG_REG_A, ARG_REG_A, ld8, 1, "LD A,A"),
    OPCODE(0xE0, ARG_IND_IMM8, ARG_REG_A, ld8, 3, "LD ($%02X),A"),
    OPCODE(0xF0, ARG_REG_A, ARG_IND_IMM8, ld8, 3, "LD A,($%02X)"),
    OPCODE(0xEA, ARG_IND_IMM16, ARG_REG_A, ld8, 4, "LD ($%04X),A"),
    OPCODE(0xFA, ARG_REG_A, ARG_IND_IMM16, ld8, 4, "LD A,($%04X)"),
    OPCODE(0x06, ARG_REG_B, ARG_IMM8, ld8, 2, "LD B,$%02X"),
    OPCODE(0xE2, ARG_IND_C, ARG_REG_A, ld8, 2, "LD (C),A"),
    OPCODE(0xF2, ARG_REG_C, ARG_IMM8, ld8, 2, "LD C,$%02X"),
    OPCODE(0x0E, ARG_REG_C, ARG_IMM8, ld8, 2, "LD C,$%02X"),
    OPCODE(0x16, ARG_REG_D, ARG_IMM8, ld8, 2, "LD D,$%02X"),
    OPCODE(0x1E, ARG_REG_E, ARG_IMM8, ld8, 2, "LD E,$%02X"),
    OPCODE(0x26, ARG_REG_H, ARG_IMM8, ld8, 2, "LD H,$%02X"),
    OPCODE(0x2E, ARG_REG_L, ARG_IMM8, ld8, 2, "LD L,$%02X"),
    OPCODE(0x36, ARG_IND_HL, ARG_IMM8, ld8, 3, "LD (HL),$%02X"),
    OPCODE(0x3E, ARG_REG_A, ARG_IMM8, ld8, 2, "LD A,$%02X"),

    OPCODE(0x01, ARG_REG_BC, ARG_IMM16, ld16, 3, "LD BC,$%04X"),
    OPCODE(0x11, ARG_REG_DE, ARG_IMM16, ld16, 3, "LD DE,$%04X"),
    OPCODE(0x21, ARG_REG_HL, ARG_IMM16, ld16, 3, "LD HL,$%04X"),
    OPCODE(0x31, ARG_REG_SP, ARG_IMM16, ld16, 3, "LD SP,$%04X"),
    OPCODE(0x08, ARG_IND_IMM16, ARG_REG_SP, ld16, 5, "LD ($%04X),SP"),
    OPCODE(0xF9, ARG_REG_SP, ARG_REG_HL, ld16, 2, "LD SP,HL"),

    OPCODE(0x80, ARG_REG_B, ARG_NONE, add_a, 1, "ADD A,B"),
    OPCODE(0x81, ARG_REG_C, ARG_NONE, add_a, 1, "ADD A,C"),
    OPCODE(0x82, ARG_REG_D, ARG_NONE, add_a, 1, "ADD A,D"),
    OPCODE(0x83, ARG_REG_E, ARG_NONE, add_a, 1, "ADD A,E"),
    OPCODE(0x84, ARG_REG_H, ARG_NONE, add_a, 1, "ADD A,H"),
    OPCODE(0x85, ARG_REG_L, ARG_NONE, add_a, 1, "ADD A,L"),
    OPCODE(0x86, ARG_IND_HL, ARG_NONE, add_a, 2, "ADD A,(HL)"),
    OPCODE(0x87, ARG_REG_A, ARG_NONE, add_a, 1, "ADD A,A"),
    OPCODE(0xC6, ARG_IMM8, ARG_NONE, add_a, 2, "ADD A,$%02X"),

    OPCODE(0xE8, ARG_IMM8, ARG_NONE, add_sp, 4, "ADD SP,$%02X"),
    OPCODE(0x09, ARG_REG_BC, ARG_NONE, add_hl, 2, "ADD HL,BC"),
    OPCODE(0x19, ARG_REG_DE, ARG_NONE, add_hl, 2, "ADD HL,DE"),
    OPCODE(0x29, ARG_REG_HL, ARG_NONE, add_hl, 2, "ADD HL,HL"),
    OPCODE(0x39, ARG_REG_SP, ARG_NONE, add_hl, 2, "ADD HL,SP"),

    OPCODE(0x88, ARG_REG_B, ARG_NONE, adc8, 1, "ADC A,B"),
    OPCODE(0x89, ARG_REG_C, ARG_NONE, adc8, 1, "ADC A,C"),
    OPCODE(0x8A, ARG_REG_D, ARG_NONE, adc8, 1, "ADC A,D"),
    OPCODE(0x8B, ARG_REG_E, ARG_NONE, adc8, 1, "ADC A,E"),
    OPCODE(0x8C, ARG_REG_H, ARG_NONE, adc8, 1, "ADC A,H"),
    OPCODE(0x8D, ARG_REG_L, ARG_NONE, adc8, 1, "ADC A,L"),
    OPCODE(0x8E, ARG_IND_HL, ARG_NONE, adc8, 2, "ADC A,(HL)"),
    OPCODE(0x8F, ARG_REG_A, ARG_NONE, adc8, 1, "ADC A,A"),
    OPCODE(0xCE, ARG_IMM8, ARG_NONE, adc8, 2, "ADC A,$%02X"),

    OPCODE(0x90, ARG_REG_B, ARG_NONE, sub8, 1, "SUB A,B"),
    OPCODE(0x91, ARG_REG_C, ARG_NONE, sub8, 1, "SUB A,C"),
    OPCODE(0x92, ARG_REG_D, ARG_NONE, sub8, 1, "SUB A,D"),
    OPCODE(0x93, ARG_REG_E, ARG_NONE, sub8, 1, "SUB A,E"),
    OPCODE(0x94, ARG_REG_H, ARG_NONE, sub8, 1, "SUB A,H"),
    OPCODE(0x95, ARG_REG_L, ARG_NONE, sub8, 1, "SUB A,L"),
    OPCODE(0x96, ARG_IND_HL, ARG_NONE, sub8, 2, "SUB A,(HL)"),
    OPCODE(0x97, ARG_REG_A, ARG_NONE, sub8, 1, "SUB A,A"),
    OPCODE(0xD6, ARG_IMM8, ARG_NONE, sub8, 2, "SUB A,$%02X"),

    OPCODE(0x98, ARG_REG_B, ARG_NONE, sbc8, 1, "SBC A,B"),
    OPCODE(0x99, ARG_REG_C, ARG_NONE, sbc8, 1, "SBC A,C"),
    OPCODE(0x9A, ARG_REG_D, ARG_NONE, sbc8, 1, "SBC A,D"),
    OPCODE(0x9B, ARG_REG_E, ARG_NONE, sbc8, 1, "SBC A,E"),
    OPCODE(0x9C, ARG_REG_H, ARG_NONE, sbc8, 1, "SBC A,H"),
    OPCODE(0x9D, ARG_REG_L, ARG_NONE, sbc8, 1, "SBC A,L"),
    OPCODE(0x9E, ARG_IND_HL, ARG_NONE, sbc8, 2, "SBC A,(HL)"),
    OPCODE(0x9F, ARG_REG_A, ARG_NONE, sbc8, 1, "SBC A,A"),
    OPCODE(0xDE, ARG_IMM8, ARG_NONE, sbc8, 2, "SBC A,$%02X"),

    OPCODE(0xA0, ARG_REG_B, ARG_NONE, and8, 1, "AND B"),
    OPCODE(0xA1, ARG_REG_C, ARG_NONE, and8, 1, "AND C"),
    OPCODE(0xA2, ARG_REG_D, ARG_NONE, and8, 1, "AND D"),
    OPCODE(0xA3, ARG_REG_E, ARG_NONE, and8, 1, "AND E"),
    OPCODE(0xA4, ARG_REG_H, ARG_NONE, and8, 1, "AND H"),
    OPCODE(0xA5, ARG_REG_L, ARG_NONE, and8, 1, "AND L"),
    OPCODE(0xA6, ARG_IND_HL, ARG_NONE, and8, 2, "AND (HL)"),
    OPCODE(0xA7, ARG_REG_A, ARG_NONE, and8, 1, "AND A"),
    OPCODE(0xE6, ARG_IMM8, ARG_NONE, and8, 2, "AND $%02X"),

    OPCODE(0xA8, ARG_REG_B, ARG_NONE, xor8, 1, "XOR B"),
    OPCODE(0xA9, ARG_REG_C, ARG_NONE, xor8, 1, "XOR C"),
    OPCODE(0xAA, ARG_REG_D, ARG_NONE, xor8, 1, "XOR D"),
    OPCODE(0xAB, ARG_REG_E, ARG_NONE, xor8, 1, "XOR E"),
    OPCODE(0xAC, ARG_REG_H, ARG_NONE, xor8, 1, "XOR H"),
    OPCODE(0xAD, ARG_REG_L, ARG_NONE, xor8, 1, "XOR L"),
    OPCODE(0xAE, ARG_IND_HL, ARG_NONE, xor8, 2, "XOR (HL)"),
    OPCODE(0xAF, ARG_REG_A, ARG_NONE, xor8, 1, "XOR A"),
    OPCODE(0xEE, ARG_IMM8, ARG_NONE, xor8, 2, "XOR $%02X"),

    OPCODE(0xB0, ARG_REG_B, ARG_NONE, or8, 1, "OR B"),
    OPCODE(0xB1, ARG_REG_C, ARG_NONE, or8, 1, "OR C"),
    OPCODE(0xB2, ARG_REG_D, ARG_NONE, or8, 1, "OR D"),
    OPCODE(0xB3, ARG_REG_E, ARG_NONE, or8, 1, "OR E"),
    OPCODE(0xB4, ARG_REG_H, ARG_NONE, or8, 1, "OR H"),
    OPCODE(0xB5, ARG_REG_L, ARG_NONE, or8, 1, "OR L"),
    OPCODE(0xB6, ARG_IND_HL, ARG_NONE, or8, 2, "OR (HL)"),
    OPCODE(0xB7, ARG_REG_A, ARG_NONE, or8, 1, "OR A"),
    OPCODE(0xF6, ARG_IMM8, ARG_NONE, or8, 2, "OR $%02X"),

    OPCODE(0xB8, ARG_REG_B, ARG_NONE, cp8, 1, "CP B"),
    OPCODE(0xB9, ARG_REG_C, ARG_NONE, cp8, 1, "CP C"),
    OPCODE(0xBA, ARG_REG_D, ARG_NONE, cp8, 1, "CP D"),
    OPCODE(0xBB, ARG_REG_E, ARG_NONE, cp8, 1, "CP E"),
    OPCODE(0xBC, ARG_REG_H, ARG_NONE, cp8, 1, "CP H"),
    OPCODE(0xBD, ARG_REG_L, ARG_NONE, cp8, 1, "CP L"),
    OPCODE(0xBE, ARG_IND_HL, ARG_NONE, cp8, 2, "CP (HL)"),
    OPCODE(0xBF, ARG_REG_A, ARG_NONE, cp8, 1, "CP A"),
    OPCODE(0xFE, ARG_IMM8, ARG_NONE, cp8, 2, "CP $%02X"),

    OPCODE(0xC1, ARG_REG_BC, ARG_NONE, pop16, 3, "POP BC"),
    OPCODE(0xD1, ARG_REG_DE, ARG_NONE, pop16, 3, "POP DE"),
    OPCODE(0xE1, ARG_REG_HL, ARG_NONE, pop16, 3, "POP HL"),
    OPCODE(0xF1, ARG_REG_AF, ARG_NONE, pop16, 3, "POP AF"),

    OPCODE(0xC5, ARG_REG_BC, ARG_NONE, push16, 4, "PUSH BC"),
    OPCODE(0xD5, ARG_REG_DE, ARG_NONE, push16, 4, "PUSH DE"),
    OPCODE(0xE5, ARG_REG_HL, ARG_NONE, push16, 4, "PUSH HL"),
    OPCODE(0xF5, ARG_REG_AF, ARG_NONE, push16, 4, "PUSH AF"),

    OPCODE(0x18, ARG_IMM8, ARG_NONE, jr8, 2, "JR $%02X"),
    OPCODE(0x28, ARG_FLAG_ZERO, ARG_IMM8, jr8_if, 2, "JR Z,$%02X"),
    OPCODE(0x38, ARG_FLAG_CARRY, ARG_IMM8, jr8_if, 2, "JR C,$%02X"),
    OPCODE(0x20, ARG_FLAG_ZERO, ARG_IMM8, jr8_ifn, 2, "JR NZ,$%02X"),
    OPCODE(0x30, ARG_FLAG_CARRY, ARG_IMM8, jr8_ifn, 2, "JR NC,$%02X"),

    OPCODE(0xC3, ARG_IMM16, ARG_NONE, jp16, 4, "JP $%04X"),
    OPCODE(0xE9, ARG_REG_HL, ARG_NONE, jp16, 1, "JP HL"),
    OPCODE(0xCA, ARG_FLAG_ZERO, ARG_IMM16, jp16_if, 3, "JP Z,$%04X"),
    OPCODE(0xDA, ARG_FLAG_CARRY, ARG_IMM16, jp16_if, 3, "JP C,$%04X"),
    OPCODE(0xC2, ARG_FLAG_ZERO, ARG_IMM16, jp16_ifn, 3, "JP NZ,$%04X"),
    OPCODE(0xD2, ARG_FLAG_CARRY, ARG_IMM16, jp16_ifn, 3, "JP NC,$%04X"),

    OPCODE(0xCD, ARG_IMM16, ARG_NONE, call, 6, "CALL $%04X"),
    OPCODE(0xCC, ARG_FLAG_ZERO, ARG_IMM16, call_if, 3, "CALL Z,$%04X"),
    OPCODE(0xDC, ARG_FLAG_CARRY, ARG_IMM16, call_if, 3, "CALL C,$%04X"),
    OPCODE(0xC4, ARG_FLAG_ZERO, ARG_IMM16, call_ifn, 3, "CALL NZ,$%04X"),
    OPCODE(0xD4, ARG_FLAG_CARRY, ARG_IMM16, call_ifn, 3, "CALL NC,$%04X"),

    OPCODE(0xC9, ARG_NONE, ARG_NONE, ret, 4, "RET"),
    OPCODE(0xD9, ARG_NONE, ARG_NONE, reti, 4, "RETI"),
    OPCODE(0xC8, ARG_FLAG_ZERO, ARG_NONE, ret_if, 2, "RET Z"),
    OPCODE(0xD8, ARG_FLAG_CARRY, ARG_NONE, ret_if, 2, "RET C"),
    OPCODE(0xC0, ARG_FLAG_ZERO, ARG_NONE, ret_ifn, 2, "RET NZ"),
    OPCODE(0xD0, ARG_FLAG_CARRY, ARG_NONE, ret_ifn, 2, "RET NC"),

    OPCODE(0xC7, ARG_RST_0, ARG_NONE, rst, 4, "RST 0"),
    OPCODE(0xCF, ARG_RST_1, ARG_NONE, rst, 4, "RST 1"),
    OPCODE(0xD7, ARG_RST_2, ARG_NONE, rst, 4, "RST 2"),
    OPCODE(0xDF, ARG_RST_3, ARG_NONE, rst, 4, "RST 3"),
    OPCODE(0xE7, ARG_RST_4, ARG_NONE, rst, 4, "RST 4"),
    OPCODE(0xEF, ARG_RST_5, ARG_NONE, rst, 4, "RST 5"),
    OPCODE(0xF7, ARG_RST_6, ARG_NONE, rst, 4, "RST 6"),
    OPCODE(0xFF, ARG_RST_7, ARG_NONE, rst, 4, "RST 7"),

    OPCODE(0x07, ARG_REG_A, ARG_NONE, rlc, 1, "RLCA"),
    OPCODE(0x17, ARG_REG_A, ARG_NONE, rla, 1, "RLA"),
    OPCODE(0x0F, ARG_REG_A, ARG_NONE, rrca, 1, "RRCA"),
    OPCODE(0x1F, ARG_REG_A, ARG_NONE, rra, 1, "RRA"),
};

const Instruction cb_opcodes[256] = {
    OPCODE(0x00, ARG_REG_B, ARG_NONE, rlc, 2, "RLC B"),
    OPCODE(0x01, ARG_REG_C, ARG_NONE, rlc, 2, "RLC C"),
    OPCODE(0x02, ARG_REG_D, ARG_NONE, rlc, 2, "RLC D"),
    OPCODE(0x03, ARG_REG_E, ARG_NONE, rlc, 2, "RLC E"),
    OPCODE(0x04, ARG_REG_H, ARG_NONE, rlc, 2, "RLC H"),
    OPCODE(0x05, ARG_REG_L, ARG_NONE, rlc, 2, "RLC L"),
    OPCODE(0x06, ARG_IND_HL, ARG_NONE, rlc, 4, "RLC (HL)"),
    OPCODE(0x07, ARG_REG_A, ARG_NONE, rlc, 2, "RLC A"),

    OPCODE(0x08, ARG_REG_B, ARG_NONE, rrc, 2, "RRC B"),
    OPCODE(0x09, ARG_REG_C, ARG_NONE, rrc, 2, "RRC C"),
    OPCODE(0x0A, ARG_REG_D, ARG_NONE, rrc, 2, "RRC D"),
    OPCODE(0x0B, ARG_REG_E, ARG_NONE, rrc, 2, "RRC E"),
    OPCODE(0x0C, ARG_REG_H, ARG_NONE, rrc, 2, "RRC H"),
    OPCODE(0x0D, ARG_REG_L, ARG_NONE, rrc, 2, "RRC L"),
    OPCODE(0x0E, ARG_IND_HL, ARG_NONE, rrc, 4, "RRC (HL)"),
    OPCODE(0x0F, ARG_REG_A, ARG_NONE, rrc, 2, "RRC A"),

    OPCODE(0x10, ARG_REG_B, ARG_NONE, rl, 2, "RL B"),
    OPCODE(0x11, ARG_REG_C, ARG_NONE, rl, 2, "RL C"),
    OPCODE(0x12, ARG_REG_D, ARG_NONE, rl, 2, "RL D"),
    OPCODE(0x13, ARG_REG_E, ARG_NONE, rl, 2, "RL E"),
    OPCODE(0x14, ARG_REG_H, ARG_NONE, rl, 2, "RL H"),
    OPCODE(0x15, ARG_REG_L, ARG_NONE, rl, 2, "RL L"),
    OPCODE(0x16, ARG_IND_HL, ARG_NONE, rl, 4, "RL (HL)"),
    OPCODE(0x17, ARG_REG_A, ARG_NONE, rl, 2, "RL A"),

    OPCODE(0x18, ARG_REG_B, ARG_NONE, rr, 2, "RR B"),
    OPCODE(0x19, ARG_REG_C, ARG_NONE, rr, 2, "RR C"),
    OPCODE(0x1A, ARG_REG_D, ARG_NONE, rr, 2, "RR D"),
    OPCODE(0x1B, ARG_REG_E, ARG_NONE, rr, 2, "RR E"),
    OPCODE(0x1C, ARG_REG_H, ARG_NONE, rr, 2, "RR H"),
    OPCODE(0x1D, ARG_REG_L, ARG_NONE, rr, 2, "RR L"),
    OPCODE(0x1E, ARG_IND_HL, ARG_NONE, rr, 4, "RR (HL)"),
    OPCODE(0x1F, ARG_REG_A, ARG_NONE, rr, 2, "RR A"),

    OPCODE(0x20, ARG_REG_B, ARG_NONE, sla, 2, "SLA B"),
    OPCODE(0x21, ARG_REG_C, ARG_NONE, sla, 2, "SLA C"),
    OPCODE(0x22, ARG_REG_D, ARG_NONE, sla, 2, "SLA D"),
    OPCODE(0x23, ARG_REG_E, ARG_NONE, sla, 2, "SLA E"),
    OPCODE(0x24, ARG_REG_H, ARG_NONE, sla, 2, "SLA H"),
    OPCODE(0x25, ARG_REG_L, ARG_NONE, sla, 2, "SLA L"),
    OPCODE(0x26, ARG_IND_HL, ARG_NONE, sla, 4, "SLA (HL)"),
    OPCODE(0x27, ARG_REG_A, ARG_NONE, sla, 2, "SLA A"),

    OPCODE(0x28, ARG_REG_B, ARG_NONE, sra, 2, "SRA B"),
    OPCODE(0x29, ARG_REG_C, ARG_NONE, sra, 2, "SRA C"),
    OPCODE(0x2A, ARG_REG_D, ARG_NONE, sra, 2, "SRA D"),
    OPCODE(0x2B, ARG_REG_E, ARG_NONE, sra, 2, "SRA E"),
    OPCODE(0x2C, ARG_REG_H, ARG_NONE, sra, 2, "SRA H"),
    OPCODE(0x2D, ARG_REG_L, ARG_NONE, sra, 2, "SRA L"),
    OPCODE(0x2E, ARG_IND_HL, ARG_NONE, sra, 4, "SRA (HL)"),
    OPCODE(0x2F, ARG_REG_A, ARG_NONE, sra, 2, "SRA A"),

    OPCODE(0x30, ARG_REG_B, ARG_NONE, swap, 2, "SWAP B"),
    OPCODE(0x31, ARG_REG_C, ARG_NONE, swap, 2, "SWAP C"),
    OPCODE(0x32, ARG_REG_D, ARG_NONE, swap, 2, "SWAP D"),
    OPCODE(0x33, ARG_REG_E, ARG_NONE, swap, 2, "SWAP E"),
    OPCODE(0x34, ARG_REG_H, ARG_NONE, swap, 2, "SWAP H"),
    OPCODE(0x35, ARG_REG_L, ARG_NONE, swap, 2, "SWAP L"),
    OPCODE(0x36, ARG_IND_HL, ARG_NONE, swap, 4, "SWAP (HL)"),
    OPCODE(0x37, ARG_REG_A, ARG_NONE, swap, 2, "SWAP A"),

    OPCODE(0x38, ARG_REG_B, ARG_NONE, srl, 2, "SRL B"),
    OPCODE(0x39, ARG_REG_C, ARG_NONE, srl, 2, "SRL C"),
    OPCODE(0x3A, ARG_REG_D, ARG_NONE, srl, 2, "SRL D"),
    OPCODE(0x3B, ARG_REG_E, ARG_NONE, srl, 2, "SRL E"),
    OPCODE(0x3C, ARG_REG_H, ARG_NONE, srl, 2, "SRL H"),
    OPCODE(0x3D, ARG_REG_L, ARG_NONE, srl, 2, "SRL L"),
    OPCODE(0x3E, ARG_IND_HL, ARG_NONE, srl, 4, "SRL (HL)"),
    OPCODE(0x3F, ARG_REG_A, ARG_NONE, srl, 2, "SRL A"),

    OPCODE(0x40, ARG_BIT_0, ARG_REG_B, bit, 2, "BIT 0,B"),
    OPCODE(0x41, ARG_BIT_0, ARG_REG_C, bit, 2, "BIT 0,C"),
    OPCODE(0x42, ARG_BIT_0, ARG_REG_D, bit, 2, "BIT 0,D"),
    OPCODE(0x43, ARG_BIT_0, ARG_REG_E, bit, 2, "BIT 0,E"),
    OPCODE(0x44, ARG_BIT_0, ARG_REG_H, bit, 2, "BIT 0,H"),
    OPCODE(0x45, ARG_BIT_0, ARG_REG_L, bit, 2, "BIT 0,L"),
    OPCODE(0x46, ARG_BIT_0, ARG_IND_HL, bit, 3, "BIT 0,(HL)"),
    OPCODE(0x47, ARG_BIT_0, ARG_REG_A, bit, 2, "BIT 0,A"),
    OPCODE(0x48, ARG_BIT_1, ARG_REG_B, bit, 2, "BIT 1,B"),
    OPCODE(0x49, ARG_BIT_1, ARG_REG_C, bit, 2, "BIT 1,C"),
    OPCODE(0x4A, ARG_BIT_1, ARG_REG_D, bit, 2, "BIT 1,D"),
    OPCODE(0x4B, ARG_BIT_1, ARG_REG_E, bit, 2, "BIT 1,E"),
    OPCODE(0x4C, ARG_BIT_1, ARG_REG_H, bit, 2, "BIT 1,H"),
    OPCODE(0x4D, ARG_BIT_1, ARG_REG_L, bit, 2, "BIT 1,L"),
    OPCODE(0x4E, ARG_BIT_1, ARG_IND_HL, bit, 3, "BIT 1,(HL)"),
    OPCODE(0x4F, ARG_BIT_1, ARG_REG_A, bit, 2, "BIT 1,A"),
    OPCODE(0x50, ARG_BIT_2, ARG_REG_B, bit, 2, "BIT 2,B"),
    OPCODE(0x51, ARG_BIT_2, ARG_REG_C, bit, 2, "BIT 2,C"),
    OPCODE(0x52, ARG_BIT_2, ARG_REG_D, bit, 2, "BIT 2,D"),
    OPCODE(0x53, ARG_BIT_2, ARG_REG_E, bit, 2, "BIT 2,E"),
    OPCODE(0x54, ARG_BIT_2, ARG_REG_H, bit, 2, "BIT 2,H"),
    OPCODE(0x55, ARG_BIT_2, ARG_REG_L, bit, 2, "BIT 2,L"),
    OPCODE(0x56, ARG_BIT_2, ARG_IND_HL, bit, 3, "BIT 2,(HL)"),
    OPCODE(0x57, ARG_BIT_2, ARG_REG_A, bit, 2, "BIT 2,A"),
    OPCODE(0x58, ARG_BIT_3, ARG_REG_B, bit, 2, "BIT 3,B"),
    OPCODE(0x59, ARG_BIT_3, ARG_REG_C, bit, 2, "BIT 3,C"),
    OPCODE(0x5A, ARG_BIT_3, ARG_REG_D, bit, 2, "BIT 3,D"),
    OPCODE(0x5B, ARG_BIT_3, ARG_REG_E, bit, 2, "BIT 3,E"),
    OPCODE(0x5C, ARG_BIT_3, ARG_REG_H, bit, 2, "BIT 3,H"),
    OPCODE(0x5D, ARG_BIT_3, ARG_REG_L, bit, 2, "BIT 3,L"),
    OPCODE(0x5E, ARG_BIT_3, ARG_IND_HL, bit, 3, "BIT 3,(HL)"),
    OPCODE(0x5F, ARG_BIT_3, ARG_REG_A, bit, 2, "BIT 3,A"),
    OPCODE(0x60, ARG_BIT_4, ARG_REG_B, bit, 2, "BIT 4,B"),
    OPCODE(0x61, ARG_BIT_4, ARG_REG_C, bit, 2, "BIT 4,C"),
    OPCODE(0x62, ARG_BIT_4, ARG_REG_D, bit, 2, "BIT 4,D"),
    OPCODE(0x63, ARG_BIT_4, ARG_REG_E, bit, 2, "BIT 4,E"),
    OPCODE(0x64, ARG_BIT_4, ARG_REG_H, bit, 2, "BIT 4,H"),
    OPCODE(0x65, ARG_BIT_4, ARG_REG_L, bit, 2, "BIT 4,L"),
    OPCODE(0x66, ARG_BIT_4, ARG_IND_HL, bit, 3, "BIT 4,(HL)"),
    OPCODE(0x67, ARG_BIT_4, ARG_REG_A, bit, 2, "BIT 4,A"),
    OPCODE(0x68, ARG_BIT_5, ARG_REG_B, bit, 2, "BIT 5,B"),
    OPCODE(0x69, ARG_BIT_5, ARG_REG_C, bit, 2, "BIT 5,C"),
    OPCODE(0x6A, ARG_BIT_5, ARG_REG_D, bit, 2, "BIT 5,D"),
    OPCODE(0x6B, ARG_BIT_5, ARG_REG_E, bit, 2, "BIT 5,E"),
    OPCODE(0x6C, ARG_BIT_5, ARG_REG_H, bit, 2, "BIT 5,H"),
    OPCODE(0x6D, ARG_BIT_5, ARG_REG_L, bit, 2, "BIT 5,L"),
    OPCODE(0x6E, ARG_BIT_5, ARG_IND_HL, bit, 3, "BIT 5,(HL)"),
    OPCODE(0x6F, ARG_BIT_5, ARG_REG_A, bit, 2, "BIT 5,A"),
    OPCODE(0x70, ARG_BIT_6, ARG_REG_B, bit, 2, "BIT 6,B"),
    OPCODE(0x71, ARG_BIT_6, ARG_REG_C, bit, 2, "BIT 6,C"),
    OPCODE(0x72, ARG_BIT_6, ARG_REG_D, bit, 2, "BIT 6,D"),
    OPCODE(0x73, ARG_BIT_6, ARG_REG_E, bit, 2, "BIT 6,E"),
    OPCODE(0x74, ARG_BIT_6, ARG_REG_H, bit, 2, "BIT 6,H"),
    OPCODE(0x75, ARG_BIT_6, ARG_REG_L, bit, 2, "BIT 6,L"),
    OPCODE(0x76, ARG_BIT_6, ARG_IND_HL, bit, 3, "BIT 6,(HL)"),
    OPCODE(0x77, ARG_BIT_6, ARG_REG_A, bit, 2, "BIT 6,A"),
    OPCODE(0x78, ARG_BIT_7, ARG_REG_B, bit, 2, "BIT 7,B"),
    OPCODE(0x79, ARG_BIT_7, ARG_REG_C, bit, 2, "BIT 7,C"),
    OPCODE(0x7A, ARG_BIT_7, ARG_REG_D, bit, 2, "BIT 7,D"),
    OPCODE(0x7B, ARG_BIT_7, ARG_REG_E, bit, 2, "BIT 7,E"),
    OPCODE(0x7C, ARG_BIT_7, ARG_REG_H, bit, 2, "BIT 7,H"),
    OPCODE(0x7D, ARG_BIT_7, ARG_REG_L, bit, 2, "BIT 7,L"),
    OPCODE(0x7E, ARG_BIT_7, ARG_IND_HL, bit, 3, "BIT 7,(HL)"),
    OPCODE(0x7F, ARG_BIT_7, ARG_REG_A, bit, 2, "BIT 7,A"),

    OPCODE(0x80, ARG_BIT_0, ARG_REG_B, res, 2, "RES 0,B"),
    OPCODE(0x81, ARG_BIT_0, ARG_REG_C, res, 2, "RES 0,C"),
    OPCODE(0x82, ARG_BIT_0, ARG_REG_D, res, 2, "RES 0,D"),
    OPCODE(0x83, ARG_BIT_0, ARG_REG_E, res, 2, "RES 0,E"),
    OPCODE(0x84, ARG_BIT_0, ARG_REG_H, res, 2, "RES 0,H"),
    OPCODE(0x85, ARG_BIT_0, ARG_REG_L, res, 2, "RES 0,L"),
    OPCODE(0x86, ARG_BIT_0, ARG_IND_HL, res, 4, "RES 0,(HL)"),
    OPCODE(0x87, ARG_BIT_0, ARG_REG_A, res, 2, "RES 0,A"),
    OPCODE(0x88, ARG_BIT_1, ARG_REG_B, res, 2, "RES 1,B"),
    OPCODE(0x89, ARG_BIT_1, ARG_REG_C, res, 2, "RES 1,C"),
    OPCODE(0x8A, ARG_BIT_1, ARG_REG_D, res, 2, "RES 1,D"),
    OPCODE(0x8B, ARG_BIT_1, ARG_REG_E, res, 2, "RES 1,E"),
    OPCODE(0x8C, ARG_BIT_1, ARG_REG_H, res, 2, "RES 1,H"),
    OPCODE(0x8D, ARG_BIT_1, ARG_REG_L, res, 2, "RES 1,L"),
    OPCODE(0x8E, ARG_BIT_1, ARG_IND_HL, res, 4, "RES 1,(HL)"),
    OPCODE(0x8F, ARG_BIT_1, ARG_REG_A, res, 2, "RES 1,A"),
    OPCODE(0x90, ARG_BIT_2, ARG_REG_B, res, 2, "RES 2,B"),
    OPCODE(0x91, ARG_BIT_2, ARG_REG_C, res, 2, "RES 2,C"),
    OPCODE(0x92, ARG_BIT_2, ARG_REG_D, res, 2, "RES 2,D"),
    OPCODE(0x93, ARG_BIT_2, ARG_REG_E, res, 2, "RES 2,E"),
    OPCODE(0x94, ARG_BIT_2, ARG_REG_H, res, 2, "RES 2,H"),
    OPCODE(0x95, ARG_BIT_2, ARG_REG_L, res, 2, "RES 2,L"),
    OPCODE(0x96, ARG_BIT_2, ARG_IND_HL, res, 4, "RES 2,(HL)"),
    OPCODE(0x97, ARG_BIT_2, ARG_REG_A, res, 2, "RES 2,A"),
    OPCODE(0x98, ARG_BIT_3, ARG_REG_B, res, 2, "RES 3,B"),
    OPCODE(0x99, ARG_BIT_3, ARG_REG_C, res, 2, "RES 3,C"),
    OPCODE(0x9A, ARG_BIT_3, ARG_REG_D, res, 2, "RES 3,D"),
    OPCODE(0x9B, ARG_BIT_3, ARG_REG_E, res, 2, "RES 3,E"),
    OPCODE(0x9C, ARG_BIT_3, ARG_REG_H, res, 2, "RES 3,H"),
    OPCODE(0x9D, ARG_BIT_3, ARG_REG_L, res, 2, "RES 3,L"),
    OPCODE(0x9E, ARG_BIT_3, ARG_IND_HL, res, 4, "RES 3,(HL)"),
    OPCODE(0x9F, ARG_BIT_3, ARG_REG_A, res, 2, "RES 3,A"),
    OPCODE(0xA0, ARG_BIT_4, ARG_REG_B, res, 2, "RES 4,B"),
    OPCODE(0xA1, ARG_BIT_4, ARG_REG_C, res, 2, "RES 4,C"),
    OPCODE(0xA2, ARG_BIT_4, ARG_REG_D, res, 2, "RES 4,D"),
    OPCODE(0xA3, ARG_BIT_4, ARG_REG_E, res, 2, "RES 4,E"),
    OPCODE(0xA4, ARG_BIT_4, ARG_REG_H, res, 2, "RES 4,H"),
    OPCODE(0xA5, ARG_BIT_4, ARG_REG_L, res, 2, "RES 4,L"),
    OPCODE(0xA6, ARG_BIT_4, ARG_IND_HL, res, 4, "RES 4,(HL)"),
    OPCODE(0xA7, ARG_BIT_4, ARG_REG_A, res, 2, "RES 4,A"),
    OPCODE(0xA8, ARG_BIT_5, ARG_REG_B, res, 2, "RES 5,B"),
    OPCODE(0xA9, ARG_BIT_5, ARG_REG_C, res, 2, "RES 5,C"),

    OPCODE(0xAA, ARG_BIT_5, ARG_REG_D, res, 2, "RES 5,D"),
    OPCODE(0xAB, ARG_BIT_5, ARG_REG_E, res, 2, "RES 5,E"),
    OPCODE(0xAC, ARG_BIT_5, ARG_REG_H, res, 2, "RES 5,H"),
    OPCODE(0xAD, ARG_BIT_5, ARG_REG_L, res, 2, "RES 5,L"),
    OPCODE(0xAE, ARG_BIT_5, ARG_IND_HL, res, 4, "RES 5,(HL)"),
    OPCODE(0xAF, ARG_BIT_5, ARG_REG_A, res, 2, "RES 5,A"),
    OPCODE(0xB0, ARG_BIT_6, ARG_REG_B, res, 2, "RES 6,B"),
    OPCODE(0xB1, ARG_BIT_6, ARG_REG_C, res, 2, "RES 6,C"),
    OPCODE(0xB2, ARG_BIT_6, ARG_REG_D, res, 2, "RES 6,D"),
    OPCODE(0xB3, ARG_BIT_6, ARG_REG_E, res, 2, "RES 6,E"),
    OPCODE(0xB4, ARG_BIT_6, ARG_REG_H, res, 2, "RES 6,H"),
    OPCODE(0xB5, ARG_BIT_6, ARG_REG_L, res, 2, "RES 6,L"),
    OPCODE(0xB6, ARG_BIT_6, ARG_IND_HL, res, 4, "RES 6,(HL)"),
    OPCODE(0xB7, ARG_BIT_6, ARG_REG_A, res, 2, "RES 6,A"),
    OPCODE(0xB8, ARG_BIT_7, ARG_REG_B, res, 2, "RES 7,B"),
    OPCODE(0xB9, ARG_BIT_7, ARG_REG_C, res, 2, "RES 7,C"),
    OPCODE(0xBA, ARG_BIT_7, ARG_REG_D, res, 2, "RES 7,D"),
    OPCODE(0xBB, ARG_BIT_7, ARG_REG_E, res, 2, "RES 7,E"),
    OPCODE(0xBC, ARG_BIT_7, ARG_REG_H, res, 2, "RES 7,H"),
    OPCODE(0xBD, ARG_BIT_7, ARG_REG_L, res, 2, "RES 7,L"),
    OPCODE(0xBE, ARG_BIT_7, ARG_IND_HL, res, 4, "RES 7,(HL)"),
    OPCODE(0xBF, ARG_BIT_7, ARG_REG_A, res, 2, "RES 7,A"),

    OPCODE(0xC0, ARG_BIT_0, ARG_REG_B, set, 2, "SET 0,B"),
    OPCODE(0xC1, ARG_BIT_0, ARG_REG_C, set, 2, "SET 0,C"),
    OPCODE(0xC2, ARG_BIT_0, ARG_REG_D, set, 2, "SET 0,D"),
    OPCODE(0xC3, ARG_BIT_0, ARG_REG_E, set, 2, "SET 0,E"),
    OPCODE(0xC4, ARG_BIT_0, ARG_REG_H, set, 2, "SET 0,H"),
    OPCODE(0xC5, ARG_BIT_0, ARG_REG_L, set, 2, "SET 0,L"),
    OPCODE(0xC6, ARG_BIT_0, ARG_IND_HL, set, 4, "SET 0,(HL)"),
    OPCODE(0xC7, ARG_BIT_0, ARG_REG_A, set, 2, "SET 0,A"),
    OPCODE(0xC8, ARG_BIT_1, ARG_REG_B, set, 2, "SET 1,B"),
    OPCODE(0xC9, ARG_BIT_1, ARG_REG_C, set, 2, "SET 1,C"),
    OPCODE(0xCA, ARG_BIT_1, ARG_REG_D, set, 2, "SET 1,D"),
    OPCODE(0xCB, ARG_BIT_1, ARG_REG_E, set, 2, "SET 1,E"),
    OPCODE(0xCC, ARG_BIT_1, ARG_REG_H, set, 2, "SET 1,H"),
    OPCODE(0xCD, ARG_BIT_1, ARG_REG_L, set, 2, "SET 1,L"),
    OPCODE(0xCE, ARG_BIT_1, ARG_IND_HL, set, 4, "SET 1,(HL)"),
    OPCODE(0xCF, ARG_BIT_1, ARG_REG_A, set, 2, "SET 1,A"),
    OPCODE(0xD0, ARG_BIT_2, ARG_REG_B, set, 2, "SET 2,B"),
    OPCODE(0xD1, ARG_BIT_2, ARG_REG_C, set, 2, "SET 2,C"),
    OPCODE(0xD2, ARG_BIT_2, ARG_REG_D, set, 2, "SET 2,D"),
    OPCODE(0xD3, ARG_BIT_2, ARG_REG_E, set, 2, "SET 2,E"),
    OPCODE(0xD4, ARG_BIT_2, ARG_REG_H, set, 2, "SET 2,H"),
    OPCODE(0xD5, ARG_BIT_2, ARG_REG_L, set, 2, "SET 2,L"),
    OPCODE(0xD6, ARG_BIT_2, ARG_IND_HL, set, 4, "SET 2,(HL)"),
    OPCODE(0xD7, ARG_BIT_2, ARG_REG_A, set, 2, "SET 2,A"),
    OPCODE(0xD8, ARG_BIT_3, ARG_REG_B, set, 2, "SET 3,B"),
    OPCODE(0xD9, ARG_BIT_3, ARG_REG_C, set, 2, "SET 3,C"),
    OPCODE(0xDA, ARG_BIT_3, ARG_REG_D, set, 2, "SET 3,D"),
    OPCODE(0xDB, ARG_BIT_3, ARG_REG_E, set, 2, "SET 3,E"),
    OPCODE(0xDC, ARG_BIT_3, ARG_REG_H, set, 2, "SET 3,H"),
    OPCODE(0xDD, ARG_BIT_3, ARG_REG_L, set, 2, "SET 3,L"),
    OPCODE(0xDE, ARG_BIT_3, ARG_IND_HL, set, 4, "SET 3,(HL)"),
    OPCODE(0xDF, ARG_BIT_3, ARG_REG_A, set, 2, "SET 3,A"),
    OPCODE(0xE0, ARG_BIT_4, ARG_REG_B, set, 2, "SET 4,B"),
    OPCODE(0xE1, ARG_BIT_4, ARG_REG_C, set, 2, "SET 4,C"),
    OPCODE(0xE2, ARG_BIT_4, ARG_REG_D, set, 2, "SET 4,D"),
    OPCODE(0xE3, ARG_BIT_4, ARG_REG_E, set, 2, "SET 4,E"),
    OPCODE(0xE4, ARG_BIT_4, ARG_REG_H, set, 2, "SET 4,H"),
    OPCODE(0xE5, ARG_BIT_4, ARG_REG_L, set, 2, "SET 4,L"),
    OPCODE(0xE6, ARG_BIT_4, ARG_IND_HL, set, 4, "SET 4,(HL)"),
    OPCODE(0xE7, ARG_BIT_4, ARG_REG_A, set, 2, "SET 4,A"),
    OPCODE(0xE8, ARG_BIT_5, ARG_REG_B, set, 2, "SET 5,B"),
    OPCODE(0xE9, ARG_BIT_5, ARG_REG_C, set, 2, "SET 5,C"),
    OPCODE(0xEA, ARG_BIT_5, ARG_REG_D, set, 2, "SET 5,D"),
    OPCODE(0xEB, ARG_BIT_5, ARG_REG_E, set, 2, "SET 5,E"),
    OPCODE(0xEC, ARG_BIT_5, ARG_REG_H, set, 2, "SET 5,H"),
    OPCODE(0xED, ARG_BIT_5, ARG_REG_L, set, 2, "SET 5,L"),
    OPCODE(0xEE, ARG_BIT_5, ARG_IND_HL, set, 4, "SET 5,(HL)"),
    OPCODE(0xEF, ARG_BIT_5, ARG_REG_A, set, 2, "SET 5,A"),
    OPCODE(0xF0, ARG_BIT_6, ARG_REG_B, set, 2, "SET 6,B"),
    OPCODE(0xF1, ARG_BIT_6, ARG_REG_C, set, 2, "SET 6,C"),
    OPCODE(0xF2, ARG_BIT_6, ARG_REG_D, set, 2, "SET 6,D"),
    OPCODE(0xF3, ARG_BIT_6, ARG_REG_E, set, 2, "SET 6,E"),
    OPCODE(0xF4, ARG_BIT_6, ARG_REG_H, set, 2, "SET 6,H"),
    OPCODE(0xF5, ARG_BIT_6, ARG_REG_L, set, 2, "SET 6,L"),
    OPCODE(0xF6, ARG_BIT_6, ARG_IND_HL, set, 4, "SET 6,(HL)"),
    OPCODE(0xF7, ARG_BIT_6, ARG_REG_A, set, 2, "SET 6,A"),
    OPCODE(0xF8, ARG_BIT_7, ARG_REG_B, set, 2, "SET 7,B"),
    OPCODE(0xF9, ARG_BIT_7, ARG_REG_C, set, 2, "SET 7,C"),
    OPCODE(0xFA, ARG_BIT_7, ARG_REG_D, set, 2, "SET 7,D"),
    OPCODE(0xFB, ARG_BIT_7, ARG_REG_E, set, 2, "SET 7,E"),
    OPCODE(0xFC, ARG_BIT_7, ARG_REG_H, set, 2, "SET 7,H"),
    OPCODE(0xFD, ARG_BIT_7, ARG_REG_L, set, 2, "SET 7,L"),
    OPCODE(0xFE, ARG_BIT_7, ARG_IND_HL, set, 4, "SET 7,(HL)"),
    OPCODE(0xFF, ARG_BIT_7, ARG_REG_A, set, 2, "SET 7,A"),
};
