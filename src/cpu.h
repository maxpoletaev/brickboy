#ifndef BRICKBOY_CPU_H
#define BRICKBOY_CPU_H

#include <stdint.h>

#include "bus.h"

// 1010 0000
// 1000 0000

typedef struct {
    uint8_t unused : 4;
    uint8_t carry : 1;
    uint8_t half_carry : 1;
    uint8_t subtract : 1;
    uint8_t zero : 1;
} gb_cpu_flags_t;

typedef struct gb_cpu_t {
    // AF register
    union {
        uint16_t af;
        struct {
            union {
                gb_cpu_flags_t flags;
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
    int8_t ime_delay;

    uint8_t stall;
    uint64_t cycle;
} gb_cpu_t;

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
} gb_operand_t;

static const char *gb_operand_names[] = {
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

typedef struct gb_instr_t gb_instr_t;

typedef struct gb_bus_t gb_bus_t;

typedef void (*cpu_handler_t)(
    gb_cpu_t *cpu,
    gb_bus_t *bus,
    const gb_instr_t *instr
);

struct gb_instr_t {
    uint8_t opcode;
    uint8_t cycles;
    gb_operand_t arg1;
    gb_operand_t arg2;
    cpu_handler_t handler;
    const char *text;
};

void gb_cpu_reset(gb_cpu_t *cpu);

void gb_cpu_step(gb_cpu_t *cpu, gb_bus_t *bus);

const gb_instr_t *gb_cpu_decode(gb_bus_t *bus, uint16_t pc);

const gb_instr_t gb_opcodes[256];

const gb_instr_t gb_cb_opcodes[256];

#endif //BRICKBOY_CPU_H
