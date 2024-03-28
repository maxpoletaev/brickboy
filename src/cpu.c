#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "cpu.h"
#include "mmu.h"

static const uint16_t reset_addr[8] = {
    0x00, 0x08, 0x10, 0x18,
    0x20, 0x28, 0x30, 0x38,
};

CPU *
cpu_new(void)
{
    CPU *cpu = xalloc(sizeof(CPU));
    cpu_reset(cpu);
    return cpu;
}

void
cpu_free(CPU **cpu)
{
    xfree(*cpu);
}

void
cpu_reset(CPU *cpu)
{
    cpu->AF = 0x01B0;
    cpu->BC = 0x0013;
    cpu->DE = 0x00D8;
    cpu->HL = 0x014D;
    cpu->SP = 0xFFFE;
    cpu->PC = 0x0000;
    cpu->IME = 0;

    cpu->halted = false;
    cpu->ime_delay = -1;
    cpu->cycle = 0;
    cpu->step = 0;
}

static uint16_t
cpu_get_operand(CPU *cpu, MMU *bus, ArgType src)
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
    case ARG_IMM_16:
        value = mmu_read16(bus, cpu->PC);
        cpu->PC += 2;
        break;
    case ARG_REG_A:
        value = cpu->A;
        break;
    case ARG_REG_B:
        value = cpu->B;
        break;
    case ARG_REG_C:
        value = cpu->C;
        break;
    case ARG_REG_D:
        value = cpu->D;
        break;
    case ARG_REG_E:
        value = cpu->E;
        break;
    case ARG_REG_H:
        value = cpu->H;
        break;
    case ARG_REG_L:
        value = cpu->L;
        break;
    case ARG_REG_AF:
        value = cpu->AF;
        break;
    case ARG_REG_BC:
        value = cpu->BC;
        break;
    case ARG_REG_DE:
        value = cpu->DE;
        break;
    case ARG_REG_HL:
        value = cpu->HL;
        break;
    case ARG_REG_SP:
        value = cpu->SP;
        break;
    case ARG_IND_C:
        addr = 0xFF00 + cpu->C;
        value = mmu_read(bus, addr);
        break;
    case ARG_IND_BC:
        value = mmu_read(bus, cpu->BC);
        break;
    case ARG_IND_DE:
        value = mmu_read(bus, cpu->DE);
        break;
    case ARG_IND_HL:
        value = mmu_read(bus, cpu->HL);
        break;
    case ARG_IND_HLI:
        value = mmu_read(bus, cpu->HL++);
        break;
    case ARG_IND_HLD:
        value = mmu_read(bus, cpu->HL--);
        break;
    case ARG_IMM_8:
        value = mmu_read(bus, cpu->PC++);
        break;
    case ARG_IND_8:
        addr = 0xFF00 + mmu_read(bus, cpu->PC++);
        value = mmu_read(bus, addr);
        break;
    case ARG_IND_16:
        addr = mmu_read16(bus, cpu->PC);
        value = mmu_read(bus, addr);
        cpu->PC += 2;
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
cpu_set_operand(CPU *cpu, MMU *bus, ArgType target, uint16_t value16)
{
    uint8_t value = (uint8_t) value16;
    uint16_t addr = 0;

    switch (target) {
    case ARG_NONE:
    case ARG_IMM_8:
    case ARG_IMM_16:
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
        cpu->A = value;
        break;
    case ARG_REG_B:
        cpu->B = value;
        break;
    case ARG_REG_C:
        cpu->C = value;
        break;
    case ARG_REG_D:
        cpu->D = value;
        break;
    case ARG_REG_E:
        cpu->E = value;
        break;
    case ARG_REG_H:
        cpu->H = value;
        break;
    case ARG_REG_L:
        cpu->L = value;
        break;
    case ARG_REG_AF:
        cpu->AF = value16;
        break;
    case ARG_REG_BC:
        cpu->BC = value16;
        break;
    case ARG_REG_DE:
        cpu->DE = value16;
        break;
    case ARG_REG_HL:
        cpu->HL = value16;
        break;
    case ARG_REG_SP:
        cpu->SP = value16;
        break;
    case ARG_IND_C:
        addr = 0xFF00 + cpu->C;
        mmu_write(bus, addr, value);
        break;
    case ARG_IND_BC:
        mmu_write(bus, cpu->BC, value);
        break;
    case ARG_IND_DE:
        mmu_write(bus, cpu->DE, value);
        break;
    case ARG_IND_HL:
        mmu_write(bus, cpu->HL, value);
        break;
    case ARG_IND_HLI:
        mmu_write(bus, cpu->HL++, value);
        break;
    case ARG_IND_HLD:
        mmu_write(bus, cpu->HL--, value);
        break;
    case ARG_IND_8:
        addr = 0xFF00 + mmu_read(bus, cpu->PC++);
        mmu_write(bus, addr, value);
        break;
    case ARG_IND_16:
        addr = mmu_read16(bus, cpu->PC);
        mmu_write(bus, addr, value);
        cpu->PC += 2;
        break;
    }
}

static uint8_t
cpu_getbit(CPU *cpu, MMU *bus, ArgType arg, uint8_t bit)
{
    uint8_t value = cpu_get_operand(cpu, bus, arg);
    return (value >> bit&7) & 1;
}

static void
cpu_setbit(CPU *cpu, MMU *bus, ArgType arg, uint8_t bit, uint8_t value)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, arg);

    bit &= 7;
    v &= ~(1 << bit);
    v |= (value & 1) << bit;

    cpu_set_operand(cpu, bus, arg, v);
}

static void
cpu_push(CPU *cpu, MMU *bus, uint16_t value)
{
    cpu->SP -= 2;
    mmu_write16(bus, cpu->SP, value);
}

static uint16_t
cpu_pop(CPU *cpu, MMU *bus)
{
    uint16_t value = mmu_read16(bus, cpu->SP);
    cpu->SP += 2;
    return value;
}

const Instruction *
cpu_decode(MMU *bus, uint16_t pc)
{
    uint8_t opcode = mmu_read(bus, pc);
    const Instruction *op = &opcodes[opcode];

    // Prefixed instructions
    if (opcode == 0xCB) {
        opcode = mmu_read(bus, pc + 1);
        op = &cb_opcodes[opcode];
    }

    return op;
}

inline bool
cpu_interrput_enabled(CPU *cpu)
{
    return cpu->IME != 0;
}

void
cpu_interrupt(CPU *cpu, MMU *bus, uint16_t addr)
{
    cpu->halted = false;

    if (cpu->IME == 0) {
        return;
    }

    cpu->IME = 0; // disable interrupts

    cpu_push(cpu, bus, cpu->PC);

    cpu->PC = addr;
}

static inline const Instruction *
cpu_execute(CPU *cpu, MMU *bus)
{
    uint8_t opcode = mmu_read(bus, cpu->PC++);
    const Instruction *op = &opcodes[opcode];

    // Prefixed instructions
    if (opcode == 0xCB) {
        opcode = mmu_read(bus, cpu->PC++);
        op = &cb_opcodes[opcode];
    }

    if (op->handler == NULL) {
        PANIC("invalid opcode: 0x%02X", opcode);
    }

    // EI takes effect one instruction later
    if (cpu->ime_delay != -1) {
        cpu->IME = (uint8_t) cpu->ime_delay;
        cpu->ime_delay = -1;
    }

    op->handler(cpu, bus, op);

    return op;
}

void
cpu_step(CPU *cpu, MMU *bus)
{
    if (cpu->step > 0) {
        cpu->step--;
        return;
    }

    if (cpu->halted) {
        return;
    }

    const Instruction *op = cpu_execute(cpu, bus);
    cpu->step = op->cycles - 1;
}

/* ----------------------------------------------------------------------------
 * Instruction Handlers
 * -------------------------------------------------------------------------- */

/* NOP
 * Flags: - - - -
 * No operation. */
static void
nop(CPU *cpu, MMU *bus, const Instruction *op)
{
    UNUSED(cpu);
    UNUSED(bus);
    UNUSED(op);
}

static void
halt(CPU *cpu, MMU *bus, const Instruction *op)
{
    UNUSED(bus);
    UNUSED(op);

    cpu->halted = true;
}

/* DAA
 * Flags: Z - 0 C
 * Decimal adjust register A. */
static void
daa(CPU *cpu, MMU *bus, const Instruction *op)
{
    UNUSED(bus);
    UNUSED(op);

    uint8_t a = cpu->A;
    uint8_t correction = cpu->flags.carry ? 0x60 : 0x00;

    if (cpu->flags.half_carry || (!cpu->flags.negative && (a&0xF) > 9)) {
        correction |= 0x06;
    }

    if (cpu->flags.carry || (!cpu->flags.negative && a > 0x99)) {
        correction |= 0x60;
    }

    if (cpu->flags.negative) {
        a -= correction;
    } else {
        a += correction;
    }

    cpu->flags.zero = a == 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = correction >= 0x60;

    cpu->A = a;
}

/* CPL
 * Flags: - 1 1 -
 * Complement A register. */
static void
cpl(CPU *cpu, MMU *bus, const Instruction *op)
{
    UNUSED(bus);
    UNUSED(op);

    cpu->A = ~cpu->A;
    cpu->flags.negative = 1;
    cpu->flags.half_carry = 1;
}

/* SCF
 * Flags: - 0 0 1
 * Set carry flag. */
static void
scf(CPU *cpu, MMU *bus, const Instruction *op)
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
ccf(CPU *cpu, MMU *bus, const Instruction *op)
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
ld8(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg2);
    cpu_set_operand(cpu, bus, op->arg1, v);
}

/* LD r16,r16
 * Flags: - - - -
 * Load 16-bit data into 16-bit register. */
static void
ld16(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint16_t v = cpu_get_operand(cpu, bus, op->arg2);
    cpu_set_operand(cpu, bus, op->arg1, v);
}

/* LD (a16),SP
 * Flags: - - - -
 * Store lower 8 bits of SP at address (a16), and the upper 8 bits at address (a16+1). */
static void
ld16_sp(CPU *cpu, MMU *bus, const Instruction *op)
{
    UNUSED(op);

    uint16_t addr = mmu_read16(bus, cpu->PC);
    uint16_t sp = cpu->SP;
    cpu->PC += 2;

    mmu_write(bus, addr, sp&0xFF);
    mmu_write(bus, addr + 1, sp >> 8);
}

/* LD HL,SP+s8
 * Flags: 0 0 H C
 * Load SP + signed 8-bit immediate into HL. */
static void
ld_hl_sp(CPU *cpu, MMU *bus, const Instruction *op)
{
    int8_t v = (int8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint16_t sp = cpu->SP;

    uint16_t sum8 = (uint16_t) (sp&0xFF) + (uint16_t) (v&0xFF);
    int32_t sum16 = (int32_t) sp + (int32_t) v;
    uint8_t half_sum = (sp&0xF) + (v&0xF);
    uint16_t r = (uint16_t) sum16;

    cpu->flags.zero = 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = half_sum > 0xF;
    cpu->flags.carry = sum8 > 0xFF;

    cpu->HL = r;
}

/* INC r8
 * Flags: Z 0 H -
 * Increment 8-bit register or memory location. */
static void
inc8(CPU *cpu, MMU *bus, const Instruction *op)
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
inc16(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint16_t v = cpu_get_operand(cpu, bus, op->arg1);
    uint16_t r = v + 1;

    cpu_set_operand(cpu, bus, op->arg1, r);
}

/* DEC r8
 * Flags: Z 1 H -
 * Decrement 8-bit register or memory location. */
static void
dec8(CPU *cpu, MMU *bus, const Instruction *op)
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
dec16(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint16_t v = cpu_get_operand(cpu, bus, op->arg1);
    uint16_t r = v - 1;

    cpu_set_operand(cpu, bus, op->arg1, r);
}

/* ADD A,r8
 * Flags: Z 0 H C
 * Add 8-bit register or memory location to A. */
static void
add_a(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t a = cpu->A;

    uint16_t sum = (uint16_t) a + (uint16_t) v;
    uint8_t half_sum = (a&0xF) + (v&0xF);
    uint8_t r = (uint8_t) sum;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = half_sum > 0xF;
    cpu->flags.carry = sum > 0xFF;

    cpu->A = r;
}

/* Add HL,r16
 * Flags: - 0 H C
 * Add 16-bit register to HL. */
static void
add_hl(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint16_t v = cpu_get_operand(cpu, bus, op->arg1);
    uint16_t hl = cpu->HL;

    uint16_t half_sum = (hl&0x0FFF) + (v&0x0FFF);
    uint32_t sum = (uint32_t) hl + (uint32_t) v;
    uint16_t r = (uint16_t) sum;

    cpu->flags.negative = 0;
    cpu->flags.half_carry = half_sum > 0x0FFF;
    cpu->flags.carry = sum > 0xFFFF;

    cpu->HL = r;
}

/* ADD SP,s8
 * Flags: 0 0 H C
 * Add 8-bit signed immediate to SP. */
static void
add_sp(CPU *cpu, MMU *bus, const Instruction *op)
{
    int8_t v = (int8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint16_t sp = cpu->SP;

    uint16_t sum8 = (uint16_t) (sp&0xFF) + (uint16_t) (v&0xFF);
    int32_t sum16 = (int32_t) sp + (int32_t) v;
    uint8_t half_sum = (sp&0xF) + (v&0xF);
    uint16_t r = (uint16_t) sum16;

    cpu->flags.zero = 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = half_sum > 0xF;
    cpu->flags.carry = sum8 > 0xFF;

    cpu->SP = r;
}

/* ADC A,r8
 * Flags: Z 0 H C
 * Add 8-bit register or memory location and carry flag to A. */
static void
adc8(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t c = cpu->flags.carry;
    uint8_t a = cpu->A;

    uint16_t sum = (uint16_t) a + (uint16_t) v + (uint16_t) c;
    uint8_t half_sum = (a&0xF) + (v&0xF) + c;
    uint8_t r = (uint8_t) sum;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = half_sum > 0xF;
    cpu->flags.carry = sum > 0xFF;

    cpu->A = r;
}

/* SUB A,r8
 * Flags: Z 1 H C
 * Subtract 8-bit register or memory location from A. */
static void
sub8(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t a = cpu->A;

    uint16_t sum = (uint16_t) a - (uint16_t) v;
    uint8_t half_sum = (a&0xF) - (v&0xF);
    uint8_t r = (uint8_t) sum;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 1;
    cpu->flags.half_carry = half_sum > 0xF;
    cpu->flags.carry = sum > 0xFF;

    cpu->A = r;
}

/* SBC A,r8
 * Flags: Z 1 H C
 * Subtract 8-bit register or memory location and carry flag from A. */
static void
sbc8(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t c = cpu->flags.carry;
    uint8_t a = cpu->A;

    uint16_t sum = (uint16_t) a - (uint16_t) v - (uint16_t) c;
    uint8_t half_sum = (a&0xF) - (v&0xF) - c;
    uint8_t r = (uint8_t) sum;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 1;
    cpu->flags.half_carry = half_sum > 0xF;
    cpu->flags.carry = sum > 0xFF;

    cpu->A = r;
}

/* AND r8
 * Flags: Z 0 1 0
 * AND 8-bit register or memory location with A. */
static void
and8(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t a = cpu->A;
    uint8_t r = a & v;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 1;
    cpu->flags.carry = 0;

    cpu->A = r;
}

/* JP a16
 * Flags: - - - -
 * Jump to 16-bit address provided by immediate operand or register. */
static void
jp16(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint16_t addr = cpu_get_operand(cpu, bus, op->arg1);
    cpu->PC = addr;
}

/* JP F,a16
 * Flags: - - - -
 * Jump to 16-bit address provided by immediate operand or register if F flag is set. */
static void
jp16_if(CPU *cpu, MMU *bus, const Instruction *op)
{
    bool flag = (bool) cpu_get_operand(cpu, bus, op->arg1);
    uint16_t addr = cpu_get_operand(cpu, bus, op->arg2);

    if (flag) {
        cpu->PC = addr;
        cpu->step += 1;
    }
}

/* JP !F,a16
 * Flags: - - - -
 * Jump to 16-bit address provided by immediate operand or register if F flag is not set. */
static void
jp16_ifn(CPU *cpu, MMU *bus, const Instruction *op)
{
    bool flag = (bool) cpu_get_operand(cpu, bus, op->arg1);
    uint16_t addr = cpu_get_operand(cpu, bus, op->arg2);

    if (!flag) {
        cpu->PC = addr;
        cpu->step += 1;
    }
}

/* JR s8
 * Flags: - - - -
 * Jump to 8-bit signed offset. */
static void
jr8(CPU *cpu, MMU *bus, const Instruction *op)
{
    int8_t offset = (int8_t) cpu_get_operand(cpu, bus, op->arg1);
    cpu->PC += offset;
}

/* JR F,s8
 * Flags: - - - -
 * Jump to 8-bit signed offset if F flag is set. */
static void
jr8_if(CPU *cpu, MMU *bus, const Instruction *op)
{
    bool flag = (bool) cpu_get_operand(cpu, bus, op->arg1);
    int8_t offset = (int8_t) cpu_get_operand(cpu, bus, op->arg2);

    if (flag) {
        cpu->PC += offset;
        cpu->step += 1;
    }
}

/* JR !F,s8
 * Flags: - - - -
 * Jump to 8-bit signed offset if F flag is not set. */
static void
jr8_ifn(CPU *cpu, MMU *bus, const Instruction *op)
{
    bool flag = (bool) cpu_get_operand(cpu, bus, op->arg1);
    int8_t offset = (int8_t) cpu_get_operand(cpu, bus, op->arg2);

    if (!flag) {
        cpu->PC += offset;
        cpu->step += 1;
    }
}

/* XOR r8
 * Flags: Z 0 0 0
 * XOR 8-bit register or memory location with A. */
static void
xor8(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t a = cpu->A;
    uint8_t r = a ^ v;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = 0;

    cpu->A = r;
}

/* OR r8
 * Flags: Z 0 0 0
 * OR 8-bit register or memory location with A. */
static void
or8(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t a = cpu->A;
    uint8_t r = a | v;

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = 0;

    cpu->A = r;
}

/* CP r8
 * Flags: Z 1 H C
 * Compare 8-bit register or memory location with A. */
static void
cp8(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t a = cpu->A;

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
push16(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint16_t v = cpu_get_operand(cpu, bus, op->arg1);
    cpu_push(cpu, bus, v);
}

/* POP r16
 * Flags: - - - -
 * Pop 16-bit register off stack. */
static void
pop16(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint16_t v = cpu_pop(cpu, bus);

    if (op->arg1 == ARG_REG_AF) {
        v &= 0xFFF0; // lower 4 bits of F are always zero
    }

    cpu_set_operand(cpu, bus, op->arg1, v);
}

/* CALL a16
 * Flags: - - - -
 * Call 16-bit address provided by immediate operand or register. */
static void
call(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint16_t addr = cpu_get_operand(cpu, bus, op->arg1);
    cpu_push(cpu, bus, cpu->PC);
    cpu->PC = addr;
}

/* CALL F,a16
 * Flags: - - - -
 * Call 16-bit address provided by immediate operand or register if F flag is set. */
static void
call_if(CPU *cpu, MMU *bus, const Instruction *op)
{
    bool flag = cpu_get_operand(cpu, bus, op->arg1) != 0;
    uint16_t addr = cpu_get_operand(cpu, bus, op->arg2);

    if (flag) {
        cpu_push(cpu, bus, cpu->PC);
        cpu->step += 3;
        cpu->PC = addr;
    }
}

/* CALL !F,a16
 * Flags: - - - -
 * Call 16-bit address provided by immediate operand or register if F flag is not set. */
static void
call_ifn(CPU *cpu, MMU *bus, const Instruction *op)
{
    bool flag = cpu_get_operand(cpu, bus, op->arg1) != 0;
    uint16_t addr = cpu_get_operand(cpu, bus, op->arg2);

    if (!flag) {
        cpu_push(cpu, bus, cpu->PC);
        cpu->step += 3;
        cpu->PC = addr;
    }
}

/* RET
 * Flags: - - - -
 * Return from subroutine. */
static void
ret(CPU *cpu, MMU *bus, const Instruction *op)
{
    UNUSED(bus);
    UNUSED(op);

    cpu->PC = cpu_pop(cpu, bus);
}

/* RET F
 * Flags: - - - -
 * Return from subroutine if F flag is set. */
static void
ret_if(CPU *cpu, MMU *bus, const Instruction *op)
{
    bool flag = cpu_get_operand(cpu, bus, op->arg1);

    if (flag) {
        cpu->PC = cpu_pop(cpu, bus);
        cpu->step += 3;
    }
}

/* RET !F
 * Flags: - - - -
 * Return from subroutine if F flag is not set. */
static void
ret_ifn(CPU *cpu, MMU *bus, const Instruction *op)
{
    bool flag = cpu_get_operand(cpu, bus, op->arg1);

    if (!flag) {
        cpu->PC = cpu_pop(cpu, bus);
        cpu->step += 3;
    }
}

/* RETI
 * Flags: - - - -
 * Return from subroutine and enable interrupts. */
static void
reti(CPU *cpu, MMU *bus, const Instruction *op)
{
    UNUSED(bus);
    UNUSED(op);

    cpu->PC = cpu_pop(cpu, bus);
    cpu->IME = 1; // looks like not delayed unlike EI
    cpu->ime_delay = -1;
}

/* RST n
 * Flags: - - - -
 * Push present address onto stack and jump to address $0000 + n,
 * where n is one of $00, $08, $10, $18, $20, $28, $30, $38. */
static void
rst(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint16_t addr = cpu_get_operand(cpu, bus, op->arg1);
    cpu_push(cpu, bus, cpu->PC);
    cpu->PC = addr;
}

/* RLC r8
 * Flags: Z 0 0 C
 * Rotate register left. */
static void
rlc(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t r = (uint8_t) ((v << 1) | (v >> 7));

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
rla(CPU *cpu, MMU *bus, const Instruction *op)
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
rl(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t r = (uint8_t) ((v << 1) | cpu->flags.carry);

    cpu->flags.zero = r == 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = (v >> 7) & 1;

    cpu_set_operand(cpu, bus, op->arg1, r);
}

/* RLCA
 * Flags: 0 0 0 C
 * Rotate register left. */
static void
rlca(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint8_t v = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    uint8_t r = (uint8_t) ((v << 1) | (v >> 7));

    cpu->flags.zero = 0;
    cpu->flags.negative = 0;
    cpu->flags.half_carry = 0;
    cpu->flags.carry = (v >> 7) & 1;

    cpu_set_operand(cpu, bus, op->arg1, r);
}

/* RRCA
 * Flags: 0 0 0 C
 * Rotate register right. */
static void
rrca(CPU *cpu, MMU *bus, const Instruction *op)
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
rrc(CPU *cpu, MMU *bus, const Instruction *op)
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
rra(CPU *cpu, MMU *bus, const Instruction *op)
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
rr(CPU *cpu, MMU *bus, const Instruction *op)
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
sla(CPU *cpu, MMU *bus, const Instruction *op)
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
sra(CPU *cpu, MMU *bus, const Instruction *op)
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
srl(CPU *cpu, MMU *bus, const Instruction *op)
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
di(CPU *cpu, MMU *bus, const Instruction *op)
{
    UNUSED(bus);
    UNUSED(op);

    cpu->IME = 0;
}

/* EI
 * Flags: - - - -
 * Enable interrupts. */
static void
ei(CPU *cpu, MMU *bus, const Instruction *op)
{
    UNUSED(bus);
    UNUSED(op);

    cpu->ime_delay = 1;
}

/* SWAP n
 * Flags: Z 0 0 0
 * Swap upper and lower nibbles of register. */
static void
swap(CPU *cpu, MMU *bus, const Instruction *op)
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
bit(CPU *cpu, MMU *bus, const Instruction *op)
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
set(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint8_t bit = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    cpu_setbit(cpu, bus, op->arg2, bit, 1);
}

/* RES b,r8
 * Flags: - - - -
 * Reset bit b in 8-bit register or memory location. */
static void
res(CPU *cpu, MMU *bus, const Instruction *op)
{
    uint8_t bit = (uint8_t) cpu_get_operand(cpu, bus, op->arg1);
    cpu_setbit(cpu, bus, op->arg2, bit, 0);
}

/* ----------------------------------------------------------------------------
 * Opcode Tables
 * -------------------------------------------------------------------------- */

#define OPCODE(op, arg1, arg2, handler, cost, text) \
    [op] = {op, cost, ARG_##arg1, ARG_##arg2, handler, text}

const Instruction opcodes[256] = {
    OPCODE(0x00, NONE, NONE, nop, 1, "NOP"),
    OPCODE(0xF3, NONE, NONE, di, 1, "DI"),
    OPCODE(0xFB, NONE, NONE, ei, 1, "EI"),
    OPCODE(0x27, NONE, NONE, daa, 1, "DAA"),
    OPCODE(0x2F, NONE, NONE, cpl, 1, "CPL"),
    OPCODE(0x37, NONE, NONE, scf, 1, "SCF"),
    OPCODE(0x3F, NONE, NONE, ccf, 1, "CCF"),
    OPCODE(0x76, NONE, NONE, halt, 1, "HALT"),

    OPCODE(0x04, REG_B, NONE, inc8, 1, "INC B"),
    OPCODE(0x0C, REG_C, NONE, inc8, 1, "INC C"),
    OPCODE(0x14, REG_D, NONE, inc8, 1, "INC D"),
    OPCODE(0x1C, REG_E, NONE, inc8, 1, "INC E"),
    OPCODE(0x24, REG_H, NONE, inc8, 1, "INC H"),
    OPCODE(0x2C, REG_L, NONE, inc8, 1, "INC L"),
    OPCODE(0x3C, REG_A, NONE, inc8, 1, "INC A"),
    OPCODE(0x34, IND_HL, NONE, inc8, 3, "INC (HL)"),

    OPCODE(0x03, REG_BC, NONE, inc16, 2, "INC BC"),
    OPCODE(0x13, REG_DE, NONE, inc16, 2, "INC DE"),
    OPCODE(0x23, REG_HL, NONE, inc16, 2, "INC HL"),
    OPCODE(0x33, REG_SP, NONE, inc16, 2, "INC SP"),

    OPCODE(0x05, REG_B, NONE, dec8, 1, "DEC B"),
    OPCODE(0x0D, REG_C, NONE, dec8, 1, "DEC C"),
    OPCODE(0x15, REG_D, NONE, dec8, 1, "DEC D"),
    OPCODE(0x1D, REG_E, NONE, dec8, 1, "DEC E"),
    OPCODE(0x25, REG_H, NONE, dec8, 1, "DEC H"),
    OPCODE(0x2D, REG_L, NONE, dec8, 1, "DEC L"),
    OPCODE(0x3D, REG_A, NONE, dec8, 1, "DEC A"),
    OPCODE(0x35, IND_HL, NONE, dec8, 3, "DEC (HL)"),

    OPCODE(0x0B, REG_BC, NONE, dec16, 2, "DEC BC"),
    OPCODE(0x1B, REG_DE, NONE, dec16, 2, "DEC DE"),
    OPCODE(0x2B, REG_HL, NONE, dec16, 2, "DEC HL"),
    OPCODE(0x3B, REG_SP, NONE, dec16, 2, "DEC SP"),

    OPCODE(0x02, IND_BC, REG_A, ld8, 2, "LD (BC),A"),
    OPCODE(0x0A, REG_A, IND_BC, ld8, 2, "LD A,(BC)"),
    OPCODE(0x12, IND_DE, REG_A, ld8, 2, "LD (DE),A"),
    OPCODE(0x1A, REG_A, IND_DE, ld8, 2, "LD A,(DE)"),
    OPCODE(0x22, IND_HLI, REG_A, ld8, 2, "LD (HL+),A"),
    OPCODE(0x2A, REG_A, IND_HLI, ld8, 2, "LD A,(HL+)"),
    OPCODE(0x32, IND_HLD, REG_A, ld8, 2, "LD (HL-),A"),
    OPCODE(0x3A, REG_A, IND_HLD, ld8, 2, "LD A,(HL-)"),
    OPCODE(0x40, REG_B, REG_B, ld8, 1, "LD B,B"),
    OPCODE(0x41, REG_B, REG_C, ld8, 1, "LD B,C"),
    OPCODE(0x42, REG_B, REG_D, ld8, 1, "LD B,D"),
    OPCODE(0x43, REG_B, REG_E, ld8, 1, "LD B,E"),
    OPCODE(0x44, REG_B, REG_H, ld8, 1, "LD B,H"),
    OPCODE(0x45, REG_B, REG_L, ld8, 1, "LD B,L"),
    OPCODE(0x46, REG_B, IND_HL, ld8, 2, "LD B,(HL)"),
    OPCODE(0x47, REG_B, REG_A, ld8, 1, "LD B,A"),
    OPCODE(0x48, REG_C, REG_B, ld8, 1, "LD C,B"),
    OPCODE(0x49, REG_C, REG_C, ld8, 1, "LD C,C"),
    OPCODE(0x4A, REG_C, REG_D, ld8, 1, "LD C,D"),
    OPCODE(0x4B, REG_C, REG_E, ld8, 1, "LD C,E"),
    OPCODE(0x4C, REG_C, REG_H, ld8, 1, "LD C,H"),
    OPCODE(0x4D, REG_C, REG_L, ld8, 1, "LD C,L"),
    OPCODE(0x4E, REG_C, IND_HL, ld8, 2, "LD C,(HL)"),
    OPCODE(0x4F, REG_C, REG_A, ld8, 1, "LD C,A"),
    OPCODE(0x50, REG_D, REG_B, ld8, 1, "LD D,B"),
    OPCODE(0x51, REG_D, REG_C, ld8, 1, "LD D,C"),
    OPCODE(0x52, REG_D, REG_D, ld8, 1, "LD D,D"),
    OPCODE(0x53, REG_D, REG_E, ld8, 1, "LD D,E"),
    OPCODE(0x54, REG_D, REG_H, ld8, 1, "LD D,H"),
    OPCODE(0x55, REG_D, REG_L, ld8, 1, "LD D,L"),
    OPCODE(0x56, REG_D, IND_HL, ld8, 2, "LD D,(HL)"),
    OPCODE(0x57, REG_D, REG_A, ld8, 1, "LD D,A"),
    OPCODE(0x58, REG_E, REG_B, ld8, 1, "LD E,B"),
    OPCODE(0x59, REG_E, REG_C, ld8, 1, "LD E,C"),
    OPCODE(0x5A, REG_E, REG_D, ld8, 1, "LD E,D"),
    OPCODE(0x5B, REG_E, REG_E, ld8, 1, "LD E,E"),
    OPCODE(0x5C, REG_E, REG_H, ld8, 1, "LD E,H"),
    OPCODE(0x5D, REG_E, REG_L, ld8, 1, "LD E,L"),
    OPCODE(0x5E, REG_E, IND_HL, ld8, 2, "LD E,(HL)"),
    OPCODE(0x5F, REG_E, REG_A, ld8, 1, "LD E,A"),
    OPCODE(0x60, REG_H, REG_B, ld8, 1, "LD H,B"),
    OPCODE(0x61, REG_H, REG_C, ld8, 1, "LD H,C"),
    OPCODE(0x62, REG_H, REG_D, ld8, 1, "LD H,D"),
    OPCODE(0x63, REG_H, REG_E, ld8, 1, "LD H,E"),
    OPCODE(0x64, REG_H, REG_H, ld8, 1, "LD H,H"),
    OPCODE(0x65, REG_H, REG_L, ld8, 1, "LD H,L"),
    OPCODE(0x66, REG_H, IND_HL, ld8, 2, "LD H,(HL)"),
    OPCODE(0x67, REG_H, REG_A, ld8, 1, "LD H,A"),
    OPCODE(0x68, REG_L, REG_B, ld8, 1, "LD L,B"),
    OPCODE(0x69, REG_L, REG_C, ld8, 1, "LD L,C"),
    OPCODE(0x6A, REG_L, REG_D, ld8, 1, "LD L,D"),
    OPCODE(0x6B, REG_L, REG_E, ld8, 1, "LD L,E"),
    OPCODE(0x6C, REG_L, REG_H, ld8, 1, "LD L,H"),
    OPCODE(0x6D, REG_L, REG_L, ld8, 1, "LD L,L"),
    OPCODE(0x6E, REG_L, IND_HL, ld8, 2, "LD L,(HL)"),
    OPCODE(0x6F, REG_L, REG_A, ld8, 1, "LD L,A"),
    OPCODE(0x70, IND_HL, REG_B, ld8, 2, "LD (HL),B"),
    OPCODE(0x71, IND_HL, REG_C, ld8, 2, "LD (HL),C"),
    OPCODE(0x72, IND_HL, REG_D, ld8, 2, "LD (HL),D"),
    OPCODE(0x73, IND_HL, REG_E, ld8, 2, "LD (HL),E"),
    OPCODE(0x74, IND_HL, REG_H, ld8, 2, "LD (HL),H"),
    OPCODE(0x75, IND_HL, REG_L, ld8, 2, "LD (HL),L"),
    OPCODE(0x77, IND_HL, REG_A, ld8, 2, "LD (HL),A"),
    OPCODE(0x78, REG_A, REG_B, ld8, 1, "LD A,B"),
    OPCODE(0x79, REG_A, REG_C, ld8, 1, "LD A,C"),
    OPCODE(0x7A, REG_A, REG_D, ld8, 1, "LD A,D"),
    OPCODE(0x7B, REG_A, REG_E, ld8, 1, "LD A,E"),
    OPCODE(0x7C, REG_A, REG_H, ld8, 1, "LD A,H"),
    OPCODE(0x7D, REG_A, REG_L, ld8, 1, "LD A,L"),
    OPCODE(0x7E, REG_A, IND_HL, ld8, 2, "LD A,(HL)"),
    OPCODE(0x7F, REG_A, REG_A, ld8, 1, "LD A,A"),
    OPCODE(0xE0, IND_8, REG_A, ld8, 3, "LD ($%02X),A"),
    OPCODE(0xF0, REG_A, IND_8, ld8, 3, "LD A,($%02X)"),
    OPCODE(0xEA, IND_16, REG_A, ld8, 4, "LD ($%04X),A"),
    OPCODE(0xFA, REG_A, IND_16, ld8, 4, "LD A,($%04X)"),
    OPCODE(0x06, REG_B, IMM_8, ld8, 2, "LD B,$%02X"),
    OPCODE(0xE2, IND_C, REG_A, ld8, 2, "LD (C),A"),
    OPCODE(0xF2, REG_A, IND_C, ld8, 2, "LD A,(C)"),
    OPCODE(0x0E, REG_C, IMM_8, ld8, 2, "LD C,$%02X"),
    OPCODE(0x16, REG_D, IMM_8, ld8, 2, "LD D,$%02X"),
    OPCODE(0x1E, REG_E, IMM_8, ld8, 2, "LD E,$%02X"),
    OPCODE(0x26, REG_H, IMM_8, ld8, 2, "LD H,$%02X"),
    OPCODE(0x2E, REG_L, IMM_8, ld8, 2, "LD L,$%02X"),
    OPCODE(0x36, IND_HL, IMM_8, ld8, 3, "LD (HL),$%02X"),
    OPCODE(0x3E, REG_A, IMM_8, ld8, 2, "LD A,$%02X"),

    OPCODE(0x01, REG_BC, IMM_16, ld16, 3, "LD BC,$%04X"),
    OPCODE(0x11, REG_DE, IMM_16, ld16, 3, "LD DE,$%04X"),
    OPCODE(0x21, REG_HL, IMM_16, ld16, 3, "LD HL,$%04X"),
    OPCODE(0x31, REG_SP, IMM_16, ld16, 3, "LD SP,$%04X"),
    OPCODE(0xF9, REG_SP, REG_HL, ld16, 2, "LD SP,HL"),
    OPCODE(0x08, NONE, NONE, ld16_sp, 5, "LD ($%04X),SP"),
    OPCODE(0xF8, IMM_8, NONE, ld_hl_sp, 3, "LD HL,SP+$%02X"),

    OPCODE(0x80, REG_B, NONE, add_a, 1, "ADD A,B"),
    OPCODE(0x81, REG_C, NONE, add_a, 1, "ADD A,C"),
    OPCODE(0x82, REG_D, NONE, add_a, 1, "ADD A,D"),
    OPCODE(0x83, REG_E, NONE, add_a, 1, "ADD A,E"),
    OPCODE(0x84, REG_H, NONE, add_a, 1, "ADD A,H"),
    OPCODE(0x85, REG_L, NONE, add_a, 1, "ADD A,L"),
    OPCODE(0x86, IND_HL, NONE, add_a, 2, "ADD A,(HL)"),
    OPCODE(0x87, REG_A, NONE, add_a, 1, "ADD A,A"),
    OPCODE(0xC6, IMM_8, NONE, add_a, 2, "ADD A,$%02X"),

    OPCODE(0xE8, IMM_8, NONE, add_sp, 4, "ADD SP,$%02X"),
    OPCODE(0x09, REG_BC, NONE, add_hl, 2, "ADD HL,BC"),
    OPCODE(0x19, REG_DE, NONE, add_hl, 2, "ADD HL,DE"),
    OPCODE(0x29, REG_HL, NONE, add_hl, 2, "ADD HL,HL"),
    OPCODE(0x39, REG_SP, NONE, add_hl, 2, "ADD HL,SP"),

    OPCODE(0x88, REG_B, NONE, adc8, 1, "ADC A,B"),
    OPCODE(0x89, REG_C, NONE, adc8, 1, "ADC A,C"),
    OPCODE(0x8A, REG_D, NONE, adc8, 1, "ADC A,D"),
    OPCODE(0x8B, REG_E, NONE, adc8, 1, "ADC A,E"),
    OPCODE(0x8C, REG_H, NONE, adc8, 1, "ADC A,H"),
    OPCODE(0x8D, REG_L, NONE, adc8, 1, "ADC A,L"),
    OPCODE(0x8E, IND_HL, NONE, adc8, 2, "ADC A,(HL)"),
    OPCODE(0x8F, REG_A, NONE, adc8, 1, "ADC A,A"),
    OPCODE(0xCE, IMM_8, NONE, adc8, 2, "ADC A,$%02X"),

    OPCODE(0x90, REG_B, NONE, sub8, 1, "SUB A,B"),
    OPCODE(0x91, REG_C, NONE, sub8, 1, "SUB A,C"),
    OPCODE(0x92, REG_D, NONE, sub8, 1, "SUB A,D"),
    OPCODE(0x93, REG_E, NONE, sub8, 1, "SUB A,E"),
    OPCODE(0x94, REG_H, NONE, sub8, 1, "SUB A,H"),
    OPCODE(0x95, REG_L, NONE, sub8, 1, "SUB A,L"),
    OPCODE(0x96, IND_HL, NONE, sub8, 2, "SUB A,(HL)"),
    OPCODE(0x97, REG_A, NONE, sub8, 1, "SUB A,A"),
    OPCODE(0xD6, IMM_8, NONE, sub8, 2, "SUB A,$%02X"),

    OPCODE(0x98, REG_B, NONE, sbc8, 1, "SBC A,B"),
    OPCODE(0x99, REG_C, NONE, sbc8, 1, "SBC A,C"),
    OPCODE(0x9A, REG_D, NONE, sbc8, 1, "SBC A,D"),
    OPCODE(0x9B, REG_E, NONE, sbc8, 1, "SBC A,E"),
    OPCODE(0x9C, REG_H, NONE, sbc8, 1, "SBC A,H"),
    OPCODE(0x9D, REG_L, NONE, sbc8, 1, "SBC A,L"),
    OPCODE(0x9E, IND_HL, NONE, sbc8, 2, "SBC A,(HL)"),
    OPCODE(0x9F, REG_A, NONE, sbc8, 1, "SBC A,A"),
    OPCODE(0xDE, IMM_8, NONE, sbc8, 2, "SBC A,$%02X"),

    OPCODE(0xA0, REG_B, NONE, and8, 1, "AND B"),
    OPCODE(0xA1, REG_C, NONE, and8, 1, "AND C"),
    OPCODE(0xA2, REG_D, NONE, and8, 1, "AND D"),
    OPCODE(0xA3, REG_E, NONE, and8, 1, "AND E"),
    OPCODE(0xA4, REG_H, NONE, and8, 1, "AND H"),
    OPCODE(0xA5, REG_L, NONE, and8, 1, "AND L"),
    OPCODE(0xA6, IND_HL, NONE, and8, 2, "AND (HL)"),
    OPCODE(0xA7, REG_A, NONE, and8, 1, "AND A"),
    OPCODE(0xE6, IMM_8, NONE, and8, 2, "AND $%02X"),

    OPCODE(0xA8, REG_B, NONE, xor8, 1, "XOR B"),
    OPCODE(0xA9, REG_C, NONE, xor8, 1, "XOR C"),
    OPCODE(0xAA, REG_D, NONE, xor8, 1, "XOR D"),
    OPCODE(0xAB, REG_E, NONE, xor8, 1, "XOR E"),
    OPCODE(0xAC, REG_H, NONE, xor8, 1, "XOR H"),
    OPCODE(0xAD, REG_L, NONE, xor8, 1, "XOR L"),
    OPCODE(0xAE, IND_HL, NONE, xor8, 2, "XOR (HL)"),
    OPCODE(0xAF, REG_A, NONE, xor8, 1, "XOR A"),
    OPCODE(0xEE, IMM_8, NONE, xor8, 2, "XOR $%02X"),

    OPCODE(0xB0, REG_B, NONE, or8, 1, "OR B"),
    OPCODE(0xB1, REG_C, NONE, or8, 1, "OR C"),
    OPCODE(0xB2, REG_D, NONE, or8, 1, "OR D"),
    OPCODE(0xB3, REG_E, NONE, or8, 1, "OR E"),
    OPCODE(0xB4, REG_H, NONE, or8, 1, "OR H"),
    OPCODE(0xB5, REG_L, NONE, or8, 1, "OR L"),
    OPCODE(0xB6, IND_HL, NONE, or8, 2, "OR (HL)"),
    OPCODE(0xB7, REG_A, NONE, or8, 1, "OR A"),
    OPCODE(0xF6, IMM_8, NONE, or8, 2, "OR $%02X"),

    OPCODE(0xB8, REG_B, NONE, cp8, 1, "CP B"),
    OPCODE(0xB9, REG_C, NONE, cp8, 1, "CP C"),
    OPCODE(0xBA, REG_D, NONE, cp8, 1, "CP D"),
    OPCODE(0xBB, REG_E, NONE, cp8, 1, "CP E"),
    OPCODE(0xBC, REG_H, NONE, cp8, 1, "CP H"),
    OPCODE(0xBD, REG_L, NONE, cp8, 1, "CP L"),
    OPCODE(0xBE, IND_HL, NONE, cp8, 2, "CP (HL)"),
    OPCODE(0xBF, REG_A, NONE, cp8, 1, "CP A"),
    OPCODE(0xFE, IMM_8, NONE, cp8, 2, "CP $%02X"),

    OPCODE(0xC1, REG_BC, NONE, pop16, 3, "POP BC"),
    OPCODE(0xD1, REG_DE, NONE, pop16, 3, "POP DE"),
    OPCODE(0xE1, REG_HL, NONE, pop16, 3, "POP HL"),
    OPCODE(0xF1, REG_AF, NONE, pop16, 3, "POP AF"),

    OPCODE(0xC5, REG_BC, NONE, push16, 4, "PUSH BC"),
    OPCODE(0xD5, REG_DE, NONE, push16, 4, "PUSH DE"),
    OPCODE(0xE5, REG_HL, NONE, push16, 4, "PUSH HL"),
    OPCODE(0xF5, REG_AF, NONE, push16, 4, "PUSH AF"),

    OPCODE(0x18, IMM_8, NONE, jr8, 2, "JR $%02X"),
    OPCODE(0x28, FLAG_ZERO, IMM_8, jr8_if, 2, "JR Z,$%02X"),
    OPCODE(0x38, FLAG_CARRY, IMM_8, jr8_if, 2, "JR C,$%02X"),
    OPCODE(0x20, FLAG_ZERO, IMM_8, jr8_ifn, 2, "JR NZ,$%02X"),
    OPCODE(0x30, FLAG_CARRY, IMM_8, jr8_ifn, 2, "JR NC,$%02X"),

    OPCODE(0xC3, IMM_16, NONE, jp16, 4, "JP $%04X"),
    OPCODE(0xE9, REG_HL, NONE, jp16, 1, "JP HL"),
    OPCODE(0xCA, FLAG_ZERO, IMM_16, jp16_if, 3, "JP Z,$%04X"),
    OPCODE(0xDA, FLAG_CARRY, IMM_16, jp16_if, 3, "JP C,$%04X"),
    OPCODE(0xC2, FLAG_ZERO, IMM_16, jp16_ifn, 3, "JP NZ,$%04X"),
    OPCODE(0xD2, FLAG_CARRY, IMM_16, jp16_ifn, 3, "JP NC,$%04X"),

    OPCODE(0xCD, IMM_16, NONE, call, 6, "CALL $%04X"),
    OPCODE(0xCC, FLAG_ZERO, IMM_16, call_if, 3, "CALL Z,$%04X"),
    OPCODE(0xDC, FLAG_CARRY, IMM_16, call_if, 3, "CALL C,$%04X"),
    OPCODE(0xC4, FLAG_ZERO, IMM_16, call_ifn, 3, "CALL NZ,$%04X"),
    OPCODE(0xD4, FLAG_CARRY, IMM_16, call_ifn, 3, "CALL NC,$%04X"),

    OPCODE(0xC9, NONE, NONE, ret, 4, "RET"),
    OPCODE(0xD9, NONE, NONE, reti, 4, "RETI"),
    OPCODE(0xC8, FLAG_ZERO, NONE, ret_if, 2, "RET Z"),
    OPCODE(0xD8, FLAG_CARRY, NONE, ret_if, 2, "RET C"),
    OPCODE(0xC0, FLAG_ZERO, NONE, ret_ifn, 2, "RET NZ"),
    OPCODE(0xD0, FLAG_CARRY, NONE, ret_ifn, 2, "RET NC"),

    OPCODE(0xC7, RST_0, NONE, rst, 4, "RST 0"),
    OPCODE(0xCF, RST_1, NONE, rst, 4, "RST 1"),
    OPCODE(0xD7, RST_2, NONE, rst, 4, "RST 2"),
    OPCODE(0xDF, RST_3, NONE, rst, 4, "RST 3"),
    OPCODE(0xE7, RST_4, NONE, rst, 4, "RST 4"),
    OPCODE(0xEF, RST_5, NONE, rst, 4, "RST 5"),
    OPCODE(0xF7, RST_6, NONE, rst, 4, "RST 6"),
    OPCODE(0xFF, RST_7, NONE, rst, 4, "RST 7"),

    OPCODE(0x07, REG_A, NONE, rlca, 1, "RLCA"),
    OPCODE(0x17, REG_A, NONE, rla, 1, "RLA"),
    OPCODE(0x0F, REG_A, NONE, rrca, 1, "RRCA"),
    OPCODE(0x1F, REG_A, NONE, rra, 1, "RRA"),
};

const Instruction cb_opcodes[256] = {
    OPCODE(0x00, REG_B, NONE, rlc, 2, "RLC B"),
    OPCODE(0x01, REG_C, NONE, rlc, 2, "RLC C"),
    OPCODE(0x02, REG_D, NONE, rlc, 2, "RLC D"),
    OPCODE(0x03, REG_E, NONE, rlc, 2, "RLC E"),
    OPCODE(0x04, REG_H, NONE, rlc, 2, "RLC H"),
    OPCODE(0x05, REG_L, NONE, rlc, 2, "RLC L"),
    OPCODE(0x06, IND_HL, NONE, rlc, 4, "RLC (HL)"),
    OPCODE(0x07, REG_A, NONE, rlc, 2, "RLC A"),

    OPCODE(0x08, REG_B, NONE, rrc, 2, "RRC B"),
    OPCODE(0x09, REG_C, NONE, rrc, 2, "RRC C"),
    OPCODE(0x0A, REG_D, NONE, rrc, 2, "RRC D"),
    OPCODE(0x0B, REG_E, NONE, rrc, 2, "RRC E"),
    OPCODE(0x0C, REG_H, NONE, rrc, 2, "RRC H"),
    OPCODE(0x0D, REG_L, NONE, rrc, 2, "RRC L"),
    OPCODE(0x0E, IND_HL, NONE, rrc, 4, "RRC (HL)"),
    OPCODE(0x0F, REG_A, NONE, rrc, 2, "RRC A"),

    OPCODE(0x10, REG_B, NONE, rl, 2, "RL B"),
    OPCODE(0x11, REG_C, NONE, rl, 2, "RL C"),
    OPCODE(0x12, REG_D, NONE, rl, 2, "RL D"),
    OPCODE(0x13, REG_E, NONE, rl, 2, "RL E"),
    OPCODE(0x14, REG_H, NONE, rl, 2, "RL H"),
    OPCODE(0x15, REG_L, NONE, rl, 2, "RL L"),
    OPCODE(0x16, IND_HL, NONE, rl, 4, "RL (HL)"),
    OPCODE(0x17, REG_A, NONE, rl, 2, "RL A"),

    OPCODE(0x18, REG_B, NONE, rr, 2, "RR B"),
    OPCODE(0x19, REG_C, NONE, rr, 2, "RR C"),
    OPCODE(0x1A, REG_D, NONE, rr, 2, "RR D"),
    OPCODE(0x1B, REG_E, NONE, rr, 2, "RR E"),
    OPCODE(0x1C, REG_H, NONE, rr, 2, "RR H"),
    OPCODE(0x1D, REG_L, NONE, rr, 2, "RR L"),
    OPCODE(0x1E, IND_HL, NONE, rr, 4, "RR (HL)"),
    OPCODE(0x1F, REG_A, NONE, rr, 2, "RR A"),

    OPCODE(0x20, REG_B, NONE, sla, 2, "SLA B"),
    OPCODE(0x21, REG_C, NONE, sla, 2, "SLA C"),
    OPCODE(0x22, REG_D, NONE, sla, 2, "SLA D"),
    OPCODE(0x23, REG_E, NONE, sla, 2, "SLA E"),
    OPCODE(0x24, REG_H, NONE, sla, 2, "SLA H"),
    OPCODE(0x25, REG_L, NONE, sla, 2, "SLA L"),
    OPCODE(0x26, IND_HL, NONE, sla, 4, "SLA (HL)"),
    OPCODE(0x27, REG_A, NONE, sla, 2, "SLA A"),

    OPCODE(0x28, REG_B, NONE, sra, 2, "SRA B"),
    OPCODE(0x29, REG_C, NONE, sra, 2, "SRA C"),
    OPCODE(0x2A, REG_D, NONE, sra, 2, "SRA D"),
    OPCODE(0x2B, REG_E, NONE, sra, 2, "SRA E"),
    OPCODE(0x2C, REG_H, NONE, sra, 2, "SRA H"),
    OPCODE(0x2D, REG_L, NONE, sra, 2, "SRA L"),
    OPCODE(0x2E, IND_HL, NONE, sra, 4, "SRA (HL)"),
    OPCODE(0x2F, REG_A, NONE, sra, 2, "SRA A"),

    OPCODE(0x30, REG_B, NONE, swap, 2, "SWAP B"),
    OPCODE(0x31, REG_C, NONE, swap, 2, "SWAP C"),
    OPCODE(0x32, REG_D, NONE, swap, 2, "SWAP D"),
    OPCODE(0x33, REG_E, NONE, swap, 2, "SWAP E"),
    OPCODE(0x34, REG_H, NONE, swap, 2, "SWAP H"),
    OPCODE(0x35, REG_L, NONE, swap, 2, "SWAP L"),
    OPCODE(0x36, IND_HL, NONE, swap, 4, "SWAP (HL)"),
    OPCODE(0x37, REG_A, NONE, swap, 2, "SWAP A"),

    OPCODE(0x38, REG_B, NONE, srl, 2, "SRL B"),
    OPCODE(0x39, REG_C, NONE, srl, 2, "SRL C"),
    OPCODE(0x3A, REG_D, NONE, srl, 2, "SRL D"),
    OPCODE(0x3B, REG_E, NONE, srl, 2, "SRL E"),
    OPCODE(0x3C, REG_H, NONE, srl, 2, "SRL H"),
    OPCODE(0x3D, REG_L, NONE, srl, 2, "SRL L"),
    OPCODE(0x3E, IND_HL, NONE, srl, 4, "SRL (HL)"),
    OPCODE(0x3F, REG_A, NONE, srl, 2, "SRL A"),

    OPCODE(0x40, BIT_0, REG_B, bit, 2, "BIT 0,B"),
    OPCODE(0x41, BIT_0, REG_C, bit, 2, "BIT 0,C"),
    OPCODE(0x42, BIT_0, REG_D, bit, 2, "BIT 0,D"),
    OPCODE(0x43, BIT_0, REG_E, bit, 2, "BIT 0,E"),
    OPCODE(0x44, BIT_0, REG_H, bit, 2, "BIT 0,H"),
    OPCODE(0x45, BIT_0, REG_L, bit, 2, "BIT 0,L"),
    OPCODE(0x46, BIT_0, IND_HL, bit, 3, "BIT 0,(HL)"),
    OPCODE(0x47, BIT_0, REG_A, bit, 2, "BIT 0,A"),
    OPCODE(0x48, BIT_1, REG_B, bit, 2, "BIT 1,B"),
    OPCODE(0x49, BIT_1, REG_C, bit, 2, "BIT 1,C"),
    OPCODE(0x4A, BIT_1, REG_D, bit, 2, "BIT 1,D"),
    OPCODE(0x4B, BIT_1, REG_E, bit, 2, "BIT 1,E"),
    OPCODE(0x4C, BIT_1, REG_H, bit, 2, "BIT 1,H"),
    OPCODE(0x4D, BIT_1, REG_L, bit, 2, "BIT 1,L"),
    OPCODE(0x4E, BIT_1, IND_HL, bit, 3, "BIT 1,(HL)"),
    OPCODE(0x4F, BIT_1, REG_A, bit, 2, "BIT 1,A"),
    OPCODE(0x50, BIT_2, REG_B, bit, 2, "BIT 2,B"),
    OPCODE(0x51, BIT_2, REG_C, bit, 2, "BIT 2,C"),
    OPCODE(0x52, BIT_2, REG_D, bit, 2, "BIT 2,D"),
    OPCODE(0x53, BIT_2, REG_E, bit, 2, "BIT 2,E"),
    OPCODE(0x54, BIT_2, REG_H, bit, 2, "BIT 2,H"),
    OPCODE(0x55, BIT_2, REG_L, bit, 2, "BIT 2,L"),
    OPCODE(0x56, BIT_2, IND_HL, bit, 3, "BIT 2,(HL)"),
    OPCODE(0x57, BIT_2, REG_A, bit, 2, "BIT 2,A"),
    OPCODE(0x58, BIT_3, REG_B, bit, 2, "BIT 3,B"),
    OPCODE(0x59, BIT_3, REG_C, bit, 2, "BIT 3,C"),
    OPCODE(0x5A, BIT_3, REG_D, bit, 2, "BIT 3,D"),
    OPCODE(0x5B, BIT_3, REG_E, bit, 2, "BIT 3,E"),
    OPCODE(0x5C, BIT_3, REG_H, bit, 2, "BIT 3,H"),
    OPCODE(0x5D, BIT_3, REG_L, bit, 2, "BIT 3,L"),
    OPCODE(0x5E, BIT_3, IND_HL, bit, 3, "BIT 3,(HL)"),
    OPCODE(0x5F, BIT_3, REG_A, bit, 2, "BIT 3,A"),
    OPCODE(0x60, BIT_4, REG_B, bit, 2, "BIT 4,B"),
    OPCODE(0x61, BIT_4, REG_C, bit, 2, "BIT 4,C"),
    OPCODE(0x62, BIT_4, REG_D, bit, 2, "BIT 4,D"),
    OPCODE(0x63, BIT_4, REG_E, bit, 2, "BIT 4,E"),
    OPCODE(0x64, BIT_4, REG_H, bit, 2, "BIT 4,H"),
    OPCODE(0x65, BIT_4, REG_L, bit, 2, "BIT 4,L"),
    OPCODE(0x66, BIT_4, IND_HL, bit, 3, "BIT 4,(HL)"),
    OPCODE(0x67, BIT_4, REG_A, bit, 2, "BIT 4,A"),
    OPCODE(0x68, BIT_5, REG_B, bit, 2, "BIT 5,B"),
    OPCODE(0x69, BIT_5, REG_C, bit, 2, "BIT 5,C"),
    OPCODE(0x6A, BIT_5, REG_D, bit, 2, "BIT 5,D"),
    OPCODE(0x6B, BIT_5, REG_E, bit, 2, "BIT 5,E"),
    OPCODE(0x6C, BIT_5, REG_H, bit, 2, "BIT 5,H"),
    OPCODE(0x6D, BIT_5, REG_L, bit, 2, "BIT 5,L"),
    OPCODE(0x6E, BIT_5, IND_HL, bit, 3, "BIT 5,(HL)"),
    OPCODE(0x6F, BIT_5, REG_A, bit, 2, "BIT 5,A"),
    OPCODE(0x70, BIT_6, REG_B, bit, 2, "BIT 6,B"),
    OPCODE(0x71, BIT_6, REG_C, bit, 2, "BIT 6,C"),
    OPCODE(0x72, BIT_6, REG_D, bit, 2, "BIT 6,D"),
    OPCODE(0x73, BIT_6, REG_E, bit, 2, "BIT 6,E"),
    OPCODE(0x74, BIT_6, REG_H, bit, 2, "BIT 6,H"),
    OPCODE(0x75, BIT_6, REG_L, bit, 2, "BIT 6,L"),
    OPCODE(0x76, BIT_6, IND_HL, bit, 3, "BIT 6,(HL)"),
    OPCODE(0x77, BIT_6, REG_A, bit, 2, "BIT 6,A"),
    OPCODE(0x78, BIT_7, REG_B, bit, 2, "BIT 7,B"),
    OPCODE(0x79, BIT_7, REG_C, bit, 2, "BIT 7,C"),
    OPCODE(0x7A, BIT_7, REG_D, bit, 2, "BIT 7,D"),
    OPCODE(0x7B, BIT_7, REG_E, bit, 2, "BIT 7,E"),
    OPCODE(0x7C, BIT_7, REG_H, bit, 2, "BIT 7,H"),
    OPCODE(0x7D, BIT_7, REG_L, bit, 2, "BIT 7,L"),
    OPCODE(0x7E, BIT_7, IND_HL, bit, 3, "BIT 7,(HL)"),
    OPCODE(0x7F, BIT_7, REG_A, bit, 2, "BIT 7,A"),

    OPCODE(0x80, BIT_0, REG_B, res, 2, "RES 0,B"),
    OPCODE(0x81, BIT_0, REG_C, res, 2, "RES 0,C"),
    OPCODE(0x82, BIT_0, REG_D, res, 2, "RES 0,D"),
    OPCODE(0x83, BIT_0, REG_E, res, 2, "RES 0,E"),
    OPCODE(0x84, BIT_0, REG_H, res, 2, "RES 0,H"),
    OPCODE(0x85, BIT_0, REG_L, res, 2, "RES 0,L"),
    OPCODE(0x86, BIT_0, IND_HL, res, 4, "RES 0,(HL)"),
    OPCODE(0x87, BIT_0, REG_A, res, 2, "RES 0,A"),
    OPCODE(0x88, BIT_1, REG_B, res, 2, "RES 1,B"),
    OPCODE(0x89, BIT_1, REG_C, res, 2, "RES 1,C"),
    OPCODE(0x8A, BIT_1, REG_D, res, 2, "RES 1,D"),
    OPCODE(0x8B, BIT_1, REG_E, res, 2, "RES 1,E"),
    OPCODE(0x8C, BIT_1, REG_H, res, 2, "RES 1,H"),
    OPCODE(0x8D, BIT_1, REG_L, res, 2, "RES 1,L"),
    OPCODE(0x8E, BIT_1, IND_HL, res, 4, "RES 1,(HL)"),
    OPCODE(0x8F, BIT_1, REG_A, res, 2, "RES 1,A"),
    OPCODE(0x90, BIT_2, REG_B, res, 2, "RES 2,B"),
    OPCODE(0x91, BIT_2, REG_C, res, 2, "RES 2,C"),
    OPCODE(0x92, BIT_2, REG_D, res, 2, "RES 2,D"),
    OPCODE(0x93, BIT_2, REG_E, res, 2, "RES 2,E"),
    OPCODE(0x94, BIT_2, REG_H, res, 2, "RES 2,H"),
    OPCODE(0x95, BIT_2, REG_L, res, 2, "RES 2,L"),
    OPCODE(0x96, BIT_2, IND_HL, res, 4, "RES 2,(HL)"),
    OPCODE(0x97, BIT_2, REG_A, res, 2, "RES 2,A"),
    OPCODE(0x98, BIT_3, REG_B, res, 2, "RES 3,B"),
    OPCODE(0x99, BIT_3, REG_C, res, 2, "RES 3,C"),
    OPCODE(0x9A, BIT_3, REG_D, res, 2, "RES 3,D"),
    OPCODE(0x9B, BIT_3, REG_E, res, 2, "RES 3,E"),
    OPCODE(0x9C, BIT_3, REG_H, res, 2, "RES 3,H"),
    OPCODE(0x9D, BIT_3, REG_L, res, 2, "RES 3,L"),
    OPCODE(0x9E, BIT_3, IND_HL, res, 4, "RES 3,(HL)"),
    OPCODE(0x9F, BIT_3, REG_A, res, 2, "RES 3,A"),
    OPCODE(0xA0, BIT_4, REG_B, res, 2, "RES 4,B"),
    OPCODE(0xA1, BIT_4, REG_C, res, 2, "RES 4,C"),
    OPCODE(0xA2, BIT_4, REG_D, res, 2, "RES 4,D"),
    OPCODE(0xA3, BIT_4, REG_E, res, 2, "RES 4,E"),
    OPCODE(0xA4, BIT_4, REG_H, res, 2, "RES 4,H"),
    OPCODE(0xA5, BIT_4, REG_L, res, 2, "RES 4,L"),
    OPCODE(0xA6, BIT_4, IND_HL, res, 4, "RES 4,(HL)"),
    OPCODE(0xA7, BIT_4, REG_A, res, 2, "RES 4,A"),
    OPCODE(0xA8, BIT_5, REG_B, res, 2, "RES 5,B"),
    OPCODE(0xA9, BIT_5, REG_C, res, 2, "RES 5,C"),

    OPCODE(0xAA, BIT_5, REG_D, res, 2, "RES 5,D"),
    OPCODE(0xAB, BIT_5, REG_E, res, 2, "RES 5,E"),
    OPCODE(0xAC, BIT_5, REG_H, res, 2, "RES 5,H"),
    OPCODE(0xAD, BIT_5, REG_L, res, 2, "RES 5,L"),
    OPCODE(0xAE, BIT_5, IND_HL, res, 4, "RES 5,(HL)"),
    OPCODE(0xAF, BIT_5, REG_A, res, 2, "RES 5,A"),
    OPCODE(0xB0, BIT_6, REG_B, res, 2, "RES 6,B"),
    OPCODE(0xB1, BIT_6, REG_C, res, 2, "RES 6,C"),
    OPCODE(0xB2, BIT_6, REG_D, res, 2, "RES 6,D"),
    OPCODE(0xB3, BIT_6, REG_E, res, 2, "RES 6,E"),
    OPCODE(0xB4, BIT_6, REG_H, res, 2, "RES 6,H"),
    OPCODE(0xB5, BIT_6, REG_L, res, 2, "RES 6,L"),
    OPCODE(0xB6, BIT_6, IND_HL, res, 4, "RES 6,(HL)"),
    OPCODE(0xB7, BIT_6, REG_A, res, 2, "RES 6,A"),
    OPCODE(0xB8, BIT_7, REG_B, res, 2, "RES 7,B"),
    OPCODE(0xB9, BIT_7, REG_C, res, 2, "RES 7,C"),
    OPCODE(0xBA, BIT_7, REG_D, res, 2, "RES 7,D"),
    OPCODE(0xBB, BIT_7, REG_E, res, 2, "RES 7,E"),
    OPCODE(0xBC, BIT_7, REG_H, res, 2, "RES 7,H"),
    OPCODE(0xBD, BIT_7, REG_L, res, 2, "RES 7,L"),
    OPCODE(0xBE, BIT_7, IND_HL, res, 4, "RES 7,(HL)"),
    OPCODE(0xBF, BIT_7, REG_A, res, 2, "RES 7,A"),

    OPCODE(0xC0, BIT_0, REG_B, set, 2, "SET 0,B"),
    OPCODE(0xC1, BIT_0, REG_C, set, 2, "SET 0,C"),
    OPCODE(0xC2, BIT_0, REG_D, set, 2, "SET 0,D"),
    OPCODE(0xC3, BIT_0, REG_E, set, 2, "SET 0,E"),
    OPCODE(0xC4, BIT_0, REG_H, set, 2, "SET 0,H"),
    OPCODE(0xC5, BIT_0, REG_L, set, 2, "SET 0,L"),
    OPCODE(0xC6, BIT_0, IND_HL, set, 4, "SET 0,(HL)"),
    OPCODE(0xC7, BIT_0, REG_A, set, 2, "SET 0,A"),
    OPCODE(0xC8, BIT_1, REG_B, set, 2, "SET 1,B"),
    OPCODE(0xC9, BIT_1, REG_C, set, 2, "SET 1,C"),
    OPCODE(0xCA, BIT_1, REG_D, set, 2, "SET 1,D"),
    OPCODE(0xCB, BIT_1, REG_E, set, 2, "SET 1,E"),
    OPCODE(0xCC, BIT_1, REG_H, set, 2, "SET 1,H"),
    OPCODE(0xCD, BIT_1, REG_L, set, 2, "SET 1,L"),
    OPCODE(0xCE, BIT_1, IND_HL, set, 4, "SET 1,(HL)"),
    OPCODE(0xCF, BIT_1, REG_A, set, 2, "SET 1,A"),
    OPCODE(0xD0, BIT_2, REG_B, set, 2, "SET 2,B"),
    OPCODE(0xD1, BIT_2, REG_C, set, 2, "SET 2,C"),
    OPCODE(0xD2, BIT_2, REG_D, set, 2, "SET 2,D"),
    OPCODE(0xD3, BIT_2, REG_E, set, 2, "SET 2,E"),
    OPCODE(0xD4, BIT_2, REG_H, set, 2, "SET 2,H"),
    OPCODE(0xD5, BIT_2, REG_L, set, 2, "SET 2,L"),
    OPCODE(0xD6, BIT_2, IND_HL, set, 4, "SET 2,(HL)"),
    OPCODE(0xD7, BIT_2, REG_A, set, 2, "SET 2,A"),
    OPCODE(0xD8, BIT_3, REG_B, set, 2, "SET 3,B"),
    OPCODE(0xD9, BIT_3, REG_C, set, 2, "SET 3,C"),
    OPCODE(0xDA, BIT_3, REG_D, set, 2, "SET 3,D"),
    OPCODE(0xDB, BIT_3, REG_E, set, 2, "SET 3,E"),
    OPCODE(0xDC, BIT_3, REG_H, set, 2, "SET 3,H"),
    OPCODE(0xDD, BIT_3, REG_L, set, 2, "SET 3,L"),
    OPCODE(0xDE, BIT_3, IND_HL, set, 4, "SET 3,(HL)"),
    OPCODE(0xDF, BIT_3, REG_A, set, 2, "SET 3,A"),
    OPCODE(0xE0, BIT_4, REG_B, set, 2, "SET 4,B"),
    OPCODE(0xE1, BIT_4, REG_C, set, 2, "SET 4,C"),
    OPCODE(0xE2, BIT_4, REG_D, set, 2, "SET 4,D"),
    OPCODE(0xE3, BIT_4, REG_E, set, 2, "SET 4,E"),
    OPCODE(0xE4, BIT_4, REG_H, set, 2, "SET 4,H"),
    OPCODE(0xE5, BIT_4, REG_L, set, 2, "SET 4,L"),
    OPCODE(0xE6, BIT_4, IND_HL, set, 4, "SET 4,(HL)"),
    OPCODE(0xE7, BIT_4, REG_A, set, 2, "SET 4,A"),
    OPCODE(0xE8, BIT_5, REG_B, set, 2, "SET 5,B"),
    OPCODE(0xE9, BIT_5, REG_C, set, 2, "SET 5,C"),
    OPCODE(0xEA, BIT_5, REG_D, set, 2, "SET 5,D"),
    OPCODE(0xEB, BIT_5, REG_E, set, 2, "SET 5,E"),
    OPCODE(0xEC, BIT_5, REG_H, set, 2, "SET 5,H"),
    OPCODE(0xED, BIT_5, REG_L, set, 2, "SET 5,L"),
    OPCODE(0xEE, BIT_5, IND_HL, set, 4, "SET 5,(HL)"),
    OPCODE(0xEF, BIT_5, REG_A, set, 2, "SET 5,A"),
    OPCODE(0xF0, BIT_6, REG_B, set, 2, "SET 6,B"),
    OPCODE(0xF1, BIT_6, REG_C, set, 2, "SET 6,C"),
    OPCODE(0xF2, BIT_6, REG_D, set, 2, "SET 6,D"),
    OPCODE(0xF3, BIT_6, REG_E, set, 2, "SET 6,E"),
    OPCODE(0xF4, BIT_6, REG_H, set, 2, "SET 6,H"),
    OPCODE(0xF5, BIT_6, REG_L, set, 2, "SET 6,L"),
    OPCODE(0xF6, BIT_6, IND_HL, set, 4, "SET 6,(HL)"),
    OPCODE(0xF7, BIT_6, REG_A, set, 2, "SET 6,A"),
    OPCODE(0xF8, BIT_7, REG_B, set, 2, "SET 7,B"),
    OPCODE(0xF9, BIT_7, REG_C, set, 2, "SET 7,C"),
    OPCODE(0xFA, BIT_7, REG_D, set, 2, "SET 7,D"),
    OPCODE(0xFB, BIT_7, REG_E, set, 2, "SET 7,E"),
    OPCODE(0xFC, BIT_7, REG_H, set, 2, "SET 7,H"),
    OPCODE(0xFD, BIT_7, REG_L, set, 2, "SET 7,L"),
    OPCODE(0xFE, BIT_7, IND_HL, set, 4, "SET 7,(HL)"),
    OPCODE(0xFF, BIT_7, REG_A, set, 2, "SET 7,A"),
};
