#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "opts.h"
#include "common.h"
#include "strbuf.h"
#include "disasm.h"
#include "serial.h"
#include "timer.h"
#include "mapper.h"
#include "mmu.h"
#include "cpu.h"
#include "rom.h"
#include "ppu.h"
#include "ui.h"
#include "mbc0.h"

static void
bitfield_test(void)
{
    union {
        struct {
            uint32_t a : 8;
            uint32_t b : 8;
            uint32_t c : 8;
            uint32_t d : 8;
        } _packed_;
        uint32_t raw;
    } test;

    test.raw = 0x01020304;

    if (test.a != 0x04 || test.b != 0x03 || test.c != 0x02 || test.d != 0x01) {
        PANIC("Byte order test failed");
    }
}

static void
print_logo(void)
{
    printf(" _          _      _    _                      \n");
    printf("| |__  _ __(_) ___| | _| |__   ___  _   _      \n");
    printf("| '_ \\| '__| |/ __| |/ / '_ \\ / _ \\| | | |  \n");
    printf("| |_) | |  | | (__|   <| |_) | (_) | |_| |     \n");
    printf("|_.__/|_|  |_|\\___|_|\\_\\_.__/ \\___/ \\__, |\n");
    printf("                                    |___/      \n");
}

static FILE*
output_file(const char *filename)
{
    if (strcmp(filename, "-") == 0 || strcmp(filename, "stdout") == 0) {
        return stdout;
    }

    if (strcmp(filename, "stderr") == 0) {
        return stderr;
    }

    if (access(filename, W_OK) != 0) { // NOLINT(misc-include-cleaner): W_OK is defined in unistd.h
        LOG("file not writable: %s", filename);
        return NULL;
    }

    return fopen(filename, "w");
}

static inline void
print_state(CPU *cpu, MMU *bus, FILE *out)
{
    fprintf(out, "A: %02X F: %02X B: %02X C: %02X D: %02X E: %02X H: %02X L: %02X SP: %04X PC: 00:%04X",
            cpu->A, cpu->F, cpu->B, cpu->C, cpu->D, cpu->E, cpu->H, cpu->L, cpu->SP, cpu->PC);

    uint8_t bytes[4] = {
        mmu_read(bus, cpu->PC),
        mmu_read(bus, cpu->PC + 1),
        mmu_read(bus, cpu->PC + 2),
        mmu_read(bus, cpu->PC + 3),
    };

    fprintf(out, " (%02X %02X %02X %02X)\n", bytes[0], bytes[1], bytes[2], bytes[3]);
}

static inline bool
handle_interrupts(CPU *cpu, MMU *mmu)
{
    static const uint8_t mask[] =  {INT_VBLANK, INT_LCD_STAT, INT_TIMER, INT_SERIAL, INT_JOYPAD};
    static const uint16_t addr[] = {0x0040, 0x0048, 0x0050, 0x0058, 0x0060};
    static_assert(ARRAY_SIZE(mask) == ARRAY_SIZE(addr), "");

    if (mmu->IF == 0 || mmu->IE == 0) {
        return false;
    }

    for (size_t i = 0; i < ARRAY_SIZE(mask); i++) {
        if ((mmu->IF & mask[i]) && (mmu->IE & mask[i])) {
            if (!cpu_interrupt(cpu, mmu, addr[i])) {
                return false;
            }

            mmu->IF &= ~mask[i];
            return true;
        }
    }

    return false;
}

static void
game_loop(CPU *cpu, MMU *mmu, Strbuf *disasm_buf, FILE *debug_out, FILE *state_out)
{
    ui_init();

    uint64_t ticks = 0;

    while (true) {
        if (ticks%4 == 0) {
            if (cpu->step == 0) {
                if (state_out != NULL) {
                    print_state(cpu, mmu, state_out);
                }

                if (debug_out != NULL) {
                    strbuf_clear(disasm_buf);
                    disasm_step(mmu, cpu, disasm_buf);
                    fputs(strbuf_get(disasm_buf), debug_out);
                    fputc('\n', debug_out);
                }
            }

            cpu_step(cpu, mmu);
        }

        ppu_step(mmu->ppu);
        timer_step(mmu->timer);

        // VBLANK interrupt requested
        if (ppu_vblank_interrupt(mmu->ppu)) {
            mmu->IF |= INT_VBLANK;
        }

        // LCD STAT interrupt requested
        if (ppu_stat_interrupt(mmu->ppu)) {
            mmu->IF |= INT_LCD_STAT;
        }

        // Timer interrupt requested
        if (timer_interrupt(mmu->timer)) {
            mmu->IF |= INT_TIMER;
        }

        serial_print(mmu->serial);
        handle_interrupts(cpu, mmu);

        if (ppu_frame_complete(mmu->ppu)) {
            ui_update_debug_view(ppu_get_vram(mmu->ppu));
            ui_update_frame_view(ppu_get_frame(mmu->ppu));
            ui_refresh();
        }

        if (ui_should_close()) {
            break;
        }
    }

    ui_close();
}

static IMapper *
get_mapper(ROM *rom)
{
    switch (rom->header->type) {
    case ROM_TYPE_ROM_ONLY:
        return mbc0_create(rom);
    default:
        LOG("unknown mapper: %02X, fallback to MBC0", rom->header->type);
        return mbc0_create(rom);
    }
}

int
main(int argc, char **argv)
{
    bitfield_test();

    Opts opts = {0};
    opts_parse(&opts, argc, argv);

    if (!opts.no_logo) {
        print_logo();
    }

    // CPU state output
    FILE *state_out _autoclose_ = NULL;
    if (opts.state_out != NULL) {
        state_out = output_file(opts.state_out);
        if (state_out == NULL) {
            LOG("failed to open state output file: %s", opts.state_out);
            exit(1);
        }
    }

    // Runtime disassembly output
    FILE *debug_out _autoclose_ = NULL;
    if (opts.debug_out != NULL) {
        debug_out = output_file(opts.debug_out);
        if (debug_out == NULL) {
            LOG("failed to open debug output file: %s", opts.debug_out);
            exit(1);
        }
    }

    ROM *rom _cleanup_(rom_free) = rom_open(opts.romfile);
    if (rom == NULL) {
        LOG("failed to open rom file: %s", opts.romfile);
        exit(1);
    }

    CPU *cpu _cleanup_(cpu_free) = cpu_new();

    PPU *ppu _cleanup_(ppu_free) = ppu_new();

    Timer *timer _cleanup_(timer_free) = timer_new();

    Serial *serial _cleanup_(serial_free) = serial_new();

    Strbuf *disasm_buf _cleanup_(strbuf_free) = strbuf_new(1024);

    IMapper *mapper _cleanup_(mapper_free) = get_mapper(rom);

    MMU *mmu _cleanup_(mmu_free) = mmu_new(mapper, serial, timer, ppu);

    game_loop(cpu, mmu, disasm_buf, debug_out, state_out);

    return 0;
}
