#ifndef BRICKBOY_CPU_H
#define BRICKBOY_CPU_H

#include <stdint.h>

#include "bus.h"

typedef struct {
    uint8_t zero : 1;
    uint8_t subtract : 1;
    uint8_t half_carry : 1;
    uint8_t carry : 1;
    uint8_t unused : 4;
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
            uint8_t b;
            uint8_t c;
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
            uint8_t h;
            uint8_t l;
        };
    };

    uint16_t sp;
    uint16_t pc;

    uint8_t halt;
    uint64_t cycles;
} gb_cpu_t;

typedef enum {
    ARG_NONE = 0,

    // immediate values
    ARG_IMM8,  // d8
    ARG_IMM16, // d16

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
    ARG_IND_BC,  // (BC)
    ARG_IND_DE,  // (DE)
    ARG_IND_HL,  // (HL)
    ARG_IND_HLI, // (HL+)
    ARG_IND_HLD, // (HL-)

    // indirect (immediate)
    ARG_IND_IMM8,  // (a8)
    ARG_IND_IMM16, // (a16)
} gb_operand_t;

typedef struct gb_opcode_t gb_opcode_t;

typedef struct gb_bus_t gb_bus_t;

typedef void (*cpu_handler_t)(
    gb_cpu_t *cpu,
    gb_bus_t *bus,
    const gb_opcode_t *op
);

struct gb_opcode_t {
    uint8_t opcode;
    uint8_t cycles;
    gb_operand_t arg1;
    gb_operand_t arg2;
    cpu_handler_t handler;
    const char *mnemonic;
};

void gb_cpu_reset(gb_cpu_t *cpu);

void gb_cpu_step(gb_cpu_t *cpu, gb_bus_t *bus);

const gb_opcode_t gb_opcodes[256];

const gb_opcode_t gb_cb_opcodes[256];

#endif //BRICKBOY_CPU_H
