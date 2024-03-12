#pragma once

#include <stdint.h>

#include "common.h"
#include "mmu.h"

typedef enum {
    INT_VBLANK = (1 << 0),
    INT_LCD_STAT = (1 << 1),
    INT_TIMER = (1 << 2),
    INT_SERIAL = (1 << 3),
    INT_JOYPAD = (1 << 4),
} Interrupt;

typedef struct CPUFlags {
    uint8_t _unused_ : 4;
    uint8_t carry : 1;
    uint8_t half_carry : 1;
    uint8_t negative : 1;
    uint8_t zero : 1;
} _packed_ CPUFlags;

typedef struct CPU {
    // AF register
    union {
        uint16_t AF;
        struct {
            union {
                CPUFlags flags;
                uint8_t F;
            };
            uint8_t A;
        } _packed_;
    };

    // BC register
    union {
        uint16_t BC;
        struct {
            uint8_t C;
            uint8_t B;
        } _packed_;
    };

    // DE register
    union {
        uint16_t DE;
        struct {
            uint8_t E;
            uint8_t D;
        } _packed_;
    };

    // HL register
    union {
        uint16_t HL;
        struct {
            uint8_t L;
            uint8_t H;
        } _packed_;
    };

    uint16_t SP;
    uint16_t PC;
    uint8_t IME;

    uint8_t step;
    uint64_t cycle;
    int8_t ime_delay;
    uint8_t halted;
} CPU;

typedef enum {
    ARG_NONE = 0,

    // immediate values
    ARG_IMM_8,  // $XX
    ARG_IMM_16, // $XXXX

    // 8-bit registers
    ARG_REG_A, // A
    ARG_REG_B, // B
    ARG_REG_C, // C
    ARG_REG_D, // D
    ARG_REG_E, // E
    ARG_REG_H, // H
    ARG_REG_L, // L

    // 16-bit registers
    ARG_REG_AF, // AF
    ARG_REG_BC, // BC
    ARG_REG_DE, // DE
    ARG_REG_HL, // HL
    ARG_REG_SP, // SP

    // indirect (register)
    ARG_IND_C,  // (C)
    ARG_IND_BC, // (BC)
    ARG_IND_DE, // (DE)
    ARG_IND_HL, // (HL)
    ARG_IND_HLI, // (HL+)
    ARG_IND_HLD, // (HL-)

    // indirect (immediate)
    ARG_IND_8,  // ($XX)
    ARG_IND_16, // ($XXXX)

    // cpu flags
    ARG_FLAG_ZERO,
    ARG_FLAG_CARRY,

    // individual bits (of the other operand)
    ARG_BIT_0,
    ARG_BIT_1,
    ARG_BIT_2,
    ARG_BIT_3,
    ARG_BIT_4,
    ARG_BIT_5,
    ARG_BIT_6,
    ARG_BIT_7,

    // reset addresses (for RST instruction)
    ARG_RST_0, // $00
    ARG_RST_1, // $08
    ARG_RST_2, // $10
    ARG_RST_3, // $18
    ARG_RST_4, // $20
    ARG_RST_5, // $28
    ARG_RST_6, // $30
    ARG_RST_7, // $38
} ArgType;

typedef struct MMU Bus;

typedef struct Instruction Instruction;

typedef void (*Handler)(CPU *cpu, MMU *bus, const Instruction *instr);

struct Instruction {
    uint8_t opcode;
    uint8_t cycles;
    ArgType arg1;
    ArgType arg2;
    Handler handler;
    const char *text;
};

CPU *cpu_new(void);

void cpu_free(CPU **cpu);

void cpu_reset(CPU *cpu);

void cpu_step(CPU *cpu, MMU *bus);

bool cpu_interrupt(CPU *cpu, MMU *bus, uint16_t pc);

const Instruction *cpu_decode(MMU *bus, uint16_t pc);

const Instruction opcodes[256];

const Instruction cb_opcodes[256];
