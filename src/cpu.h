#ifndef BRICKBOY_CPU_H
#define BRICKBOY_CPU_H

#include <stdint.h>

#include "bus.h"

typedef struct {
    uint8_t unused : 4;
    uint8_t carry : 1;
    uint8_t half_carry : 1;
    uint8_t negative : 1;
    uint8_t zero : 1;
} CPUFlags;

typedef struct CPU {
    // AF register
    union {
        uint16_t af;
        struct {
            union {
                CPUFlags flags;
                uint8_t f;
            };
            uint8_t a;
        };
    };

    // BC register
    union {
        uint16_t bc;
        struct {
            uint8_t c;
            uint8_t b;
        };
    };

    // DE register
    union {
        uint16_t de;
        struct {
            uint8_t e;
            uint8_t d;
        };
    };

    // HL register
    union {
        uint16_t hl;
        struct {
            uint8_t l;
            uint8_t h;
        };
    };

    uint16_t sp;
    uint16_t pc;
    uint8_t ime;
    uint8_t halted;

    uint64_t cycle;
    int8_t ime_delay;
    uint8_t remaining;
} CPU;

typedef enum {
    ARG_NONE = 0,

    // immediate values
    ARG_IMM8,  // $XX
    ARG_IMM16, // $XXXX

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
    ARG_IND_IMM8,  // ($XX)
    ARG_IND_IMM16, // ($XXXX)

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
} Operand;

typedef enum {
    INTERRUPT_VBLANK = 0,
    INTERRUPT_LCDSTAT = 1,
    INTERRUPT_TIMER = 2,
    INTERRUPT_SERIAL = 3,
    INTERRUPT_JOYPAD = 4,
} Interrupt;

typedef struct Instruction Instruction;

typedef struct Bus Bus;

typedef void (*Handler)(
    CPU *cpu,
    Bus *bus,
    const Instruction *instr
);

struct Instruction {
    uint8_t opcode;
    uint8_t cycles;
    Operand arg1;
    Operand arg2;
    Handler handler;
    const char *text;
};

void cpu_reset(CPU *cpu);

void cpu_step(CPU *cpu, Bus *bus);

void cpu_interrupt(CPU *cpu, Bus *bus, Interrupt interrupt);

const Instruction *cpu_decode(Bus *bus, uint16_t pc);

const Instruction opcodes[256];

const Instruction cb_opcodes[256];

#endif //BRICKBOY_CPU_H
