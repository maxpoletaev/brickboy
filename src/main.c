#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "shared.h"
#include "mapper.h"
#include "disasm.h"
#include "bus.h"
#include "cpu.h"
#include "rom.h"

static struct gb_opts_t {
    char *filename;
    uint16_t breakpoint;
    bool print_serial;
    bool debug_log;
    bool state_log;
    bool slow;
} gb_opts;

static void
gb_print_logo(void)
{
    printf(" _          _      _    _                \n");
    printf("| |__  _ __(_) ___| | _| |__   ___  _   _\n");
    printf("| '_ \\| '__| |/ __| |/ / '_ \\ / _ \\| | | |\n");
    printf("| |_) | |  | | (__|   <| |_) | (_) | |_| |\n");
    printf("|_.__/|_|  |_|\\___|_|\\_\\_.__/ \\___/ \\__, |\n");
    printf("                                    |___/\n");
}

static void
gb_print_usage(void)
{
    printf("usage: gb [-d] <rom>\n");
}

static void
gb_print_help(void)
{
    gb_print_usage();
    printf("\n");
    printf("options:\n");
    printf("  -h      print this help message\n");
    printf("  -d      enable debug mode (disassembles each instruction before executing it)\n");
    printf("  -l      enable state log mode (logs cpu state after each instruction)\n");
    printf("  -s      print serial output to stdout\n");
    printf("  -y      slow down the emulation speed\n");
    printf("  -bAAAA  set breakpoint at address \n");
    printf("\n");
    printf("examples:\n");
    printf("  gb -d roms/tetris.gb\n");
    printf("\n");
}

static void
gb_args_parse(int argc, char **argv)
{
    int opt;

    while ((opt = getopt(argc, argv, "ylhdsb:")) != -1) {
        switch (opt) {
        case 'd':
            gb_opts.debug_log = true;
            break;
        case 's':
            gb_opts.print_serial = true;
            break;
        case 'h':
            gb_print_help();
            exit(0);
        case 'b':
            gb_opts.breakpoint = (uint16_t) strtoul(optarg, NULL, 16);
            break;
        case 'l':
            gb_opts.state_log = true;
            break;
        case 'y':
            gb_opts.slow = true;
            break;
        default:
            gb_print_usage();
            exit(1);
        }
    }

    if (optind >= argc) {
        gb_print_usage();
        exit(1);
    }

    gb_opts.filename = argv[optind];
}

static inline void
gb_print_state(gb_cpu_t *cpu, gb_bus_t *bus)
{
    printf("A: %02X F: %02X B: %02X C: %02X D: %02X E: %02X H: %02X L: %02X SP: %04X PC: 00:%04X",
           cpu->a, cpu->f, cpu->b, cpu->c, cpu->d, cpu->e, cpu->h, cpu->l, cpu->sp, cpu->pc);

    uint8_t bytes[4] = {
            gb_bus_read(bus, cpu->pc),
            gb_bus_read(bus, cpu->pc + 1),
            gb_bus_read(bus, cpu->pc + 2),
            gb_bus_read(bus, cpu->pc + 3),
    };

    printf(" (%02X %02X %02X %02X)\n", bytes[0], bytes[1], bytes[2], bytes[3]);
}

int
main(int argc, char **argv)
{
    gb_mapper_t mapper;
    gb_rom_t rom;
    gb_bus_t bus;
    gb_cpu_t cpu;
    int ret = 0;

    gb_args_parse(argc, argv);
    gb_print_logo();

    if (access(gb_opts.filename, R_OK) != 0) { // NOLINT(misc-include-cleaner): R_OK is defined in unistd.h
        GB_LOG("file not found: %s", gb_opts.filename);
        ret = 1;
        goto cleanup;
    }

    if (gb_rom_open(&rom, gb_opts.filename) != GB_OK) {
        GB_TRACE("failed to open rom file");
        ret = 1;
        goto cleanup;
    }

    if (gb_mapper_init(&mapper, &rom) != GB_OK) {
        GB_TRACE("failed to initialize mapper");
        ret = 1;
        goto cleanup;
    }

    char *title = gb_cstring(rom.header->title, sizeof(rom.header->title));
    GB_LOG("rom loaded: %s (%s)", title, mapper.name);
    free(title);

    bus.mapper = &mapper;
    gb_bus_reset(&bus);
    gb_cpu_reset(&cpu);

    if (gb_opts.print_serial)
        GB_LOG("printing serial output is enabled");

    while (1) {
        if (cpu.stall == 0 && gb_opts.debug_log) {
            gb_disasm_step(&bus, &cpu, stdout);
        }

        if (cpu.stall == 0 && gb_opts.state_log) {
            gb_print_state(&cpu, &bus);
        }

        gb_cpu_step(&cpu, &bus);
        gb_bus_step(&bus);

        if (bus.serial_ctrl == 0x81 && gb_opts.print_serial) {
            printf("%c", bus.serial_data);
            bus.serial_ctrl = 0x01;
        }

        if (gb_opts.breakpoint > 0 && cpu.pc == gb_opts.breakpoint) {
            printf("breakpoint reached at 0x%04X\n", cpu.pc);
            break;
        }

        if (gb_opts.slow)
            usleep(1000);
    }

cleanup:
    gb_mapper_free(&mapper);
    gb_rom_free(&rom);

    return ret;
}
