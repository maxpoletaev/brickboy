#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "opts.h"
#include "common.h"
#include "str.h"
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
#include "mbc1.h"
#include "joypad.h"
#include "interrupt.h"

static void
bitfield_test(void)
{
    union {
        struct {
            uint32_t d : 8;
            uint32_t c : 8;
            uint32_t b : 8;
            uint32_t a : 8;
        } _packed_;
        uint32_t raw;
    } test;

    test.a = 0x01;
    test.b = 0x02;
    test.c = 0x03;
    test.d = 0x04;

    if (test.raw != 0x01020304) {
        PANIC("byte order test failed: 0x%08X", test.raw);
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
    if (strcmp(filename, "-") == 0 ||
        strcmp(filename, "stdout") == 0) {
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
gb_print_state(CPU *cpu, MMU *bus, FILE *out)
{
    fprintf(out, "A: %02X F: %02X B: %02X C: %02X D: %02X E: %02X H: %02X L: %02X SP: %04X PC: 00:%04X",
            cpu->A, cpu->F, cpu->B, cpu->C, cpu->D, cpu->E, cpu->H, cpu->L, cpu->SP, cpu->PC);

    uint8_t bytes[4] = {
        mmu_read(bus, cpu->PC),
        mmu_read(bus, cpu->PC + 1),
        mmu_read(bus, cpu->PC + 2),
        mmu_read(bus, cpu->PC + 3),
    };

    fprintf(out, " (%02X %02X %02X %02X)", bytes[0], bytes[1], bytes[2], bytes[3]);
    fputc('\n', out);

    if (ferror(out)) {
        PANIC("%s", strerror(errno));
    }
}

static inline void
gb_handle_interrupts(CPU *cpu, MMU *mmu)
{
    static const uint8_t ints[] =  {INT_VBLANK, INT_LCD_STAT, INT_TIMER, INT_SERIAL, INT_JOYPAD};
    static const uint16_t addrs[] = {0x0040, 0x0048, 0x0050, 0x0058, 0x0060};
    static_assert(ARRAY_SIZE(ints) == ARRAY_SIZE(addrs), "");

    if (mmu->IF == 0 || mmu->IE == 0) {
        return;
    }

    for (size_t i = 0; i < ARRAY_SIZE(ints); i++) {
        bool requested = mmu_interrupt_requested(mmu, ints[i]);
        bool enabled = mmu_interrupt_enabled(mmu, ints[i]);

        if (requested && enabled) {
            if (cpu_interrput_enabled(cpu)) {
                mmu_clear_interrupt(mmu, ints[i]);
                cpu_interrupt(cpu, mmu, addrs[i]);
            }

            return;
        }
    }
}

static inline void
gb_handle_input(MMU *mmu)
{
    static const JoypadButton buttons[] = {
        JOYPAD_RIGHT, JOYPAD_LEFT, JOYPAD_UP, JOYPAD_DOWN,
        JOYPAD_A, JOYPAD_B, JOYPAD_SELECT, JOYPAD_START,
    };

    mmu_clear_interrupt(mmu, INT_JOYPAD);
    joypad_clear(mmu->joypad);

    for (size_t i = 0; i < ARRAY_SIZE(buttons); i++) {
        if (ui_button_pressed(buttons[i])) {
            joypad_press(mmu->joypad, buttons[i]);
            mmu_set_interrupt(mmu, INT_JOYPAD);
        }
    }
}

static void
gb_run_loop(CPU *cpu, MMU *mmu, FILE *debug_out, FILE *state_out)
{
    ui_init();

    uint64_t ticks = 0;
    String disasm_str _cleanup_(str_free) = str_new_size(1024);

    while (true) {
        if (ticks%4 == 0) {
            if (cpu->step == 0 && !cpu->halted) {
                if (state_out != NULL) {
                    gb_print_state(cpu, mmu, state_out);
                }

                if (debug_out != NULL) {
                    disasm_str = str_trunc(disasm_str, 0);
                    disasm_str = disasm_step(mmu, cpu, disasm_str);
                    fputs(disasm_str.ptr, debug_out);
                    fputc('\n', debug_out);

                    if (ferror(debug_out)) {
                        PANIC("%s", strerror(errno));
                    }
                }
            }

            // CPU is clocked at 1/4 of the master clock.
            gb_handle_interrupts(cpu, mmu);
            cpu_step(cpu, mmu);
        }

        // PPU and timer run at the master clock.
        timer_step(mmu->timer);
        ppu_step(mmu->ppu);
        mmu_dma_step(mmu);

        // VBLANK interrupt requested
        if (ppu_vblank_interrupt(mmu->ppu)) {
            mmu_set_interrupt(mmu, INT_VBLANK);

            ui_update_debug_view(ppu_get_vram(mmu->ppu));
            ui_update_frame_view(ppu_get_frame(mmu->ppu));
            ui_refresh();

            if (ui_reset_pressed()) {
                LOG("RESET pressed");
                cpu_reset(cpu);
                mmu_reset(mmu);
            }

            if (ui_should_close()) {
                break;
            }

            gb_handle_input(mmu);
        }

        // LCD STAT interrupt requested
        if (ppu_stat_interrupt(mmu->ppu)) {
            mmu_set_interrupt(mmu, INT_LCD_STAT);
        }

        // Timer interrupt requested
        if (timer_interrupt(mmu->timer)) {
            mmu_set_interrupt(mmu, INT_TIMER);
        }

        ticks++;
    }

    ui_close();
}

static IMapper *
gb_get_mapper(ROM *rom)
{
    switch (rom->header->type) {
    case ROM_TYPE_ROM_ONLY:
        return mbc0_new(rom);
    case ROM_TYPE_MBC1:
    case ROM_TYPE_MBC1_RAM:
    case ROM_TYPE_MBC1_RAM_BATT:
        return mbc1_new(rom);
    default:
        LOG("unknown mapper: %02X", rom->header->type);
        return NULL;
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

    IMapper *mapper _cleanup_(mapper_free) = gb_get_mapper(rom);
    if (mapper == NULL) {
        LOG("failed to load rom: %s", opts.romfile);
        exit(1);
    }

    String save_file _cleanup_(str_free) = str_new_from(opts.romfile);
    save_file = str_add(save_file, ".battery");

    // Load battery-backed RAM
    if (mapper_load_state(mapper, save_file.ptr) != RET_OK) {
        LOG("failed to load state file: %s", save_file.ptr);
        exit(1);
    }

    // Initialize components
    CPU *cpu _cleanup_(cpu_free) = cpu_new();
    PPU *ppu _cleanup_(ppu_free) = ppu_new();
    Timer *timer _cleanup_(timer_free) = timer_new();
    Serial *serial _cleanup_(serial_free) = serial_new();
    Joypad *joypad _cleanup_(joypad_free) = joypad_new();
    MMU *mmu _cleanup_(mmu_free) = mmu_new(mapper, serial, timer, ppu, joypad);

    // Main loop
    gb_run_loop(cpu, mmu, debug_out, state_out);

    // Save battery-backed RAM
    if (mapper_save_state(mapper, save_file.ptr) != RET_OK) {
        LOG("failed to save state file: %s", save_file.ptr);
        exit(1);
    }

    return 0;
}
