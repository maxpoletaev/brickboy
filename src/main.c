#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "shared.h"
#include "mapper.h"
#include "disasm.h"
#include "bus.h"
#include "cpu.h"
#include "rom.h"

static struct gb_opts_t {
    char *filename;
    bool debug;
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
    printf("  -d  enable debug mode (disassembles each instruction before executing it)\n");
    printf("\n");
    printf("examples:\n");
    printf("  gb -d roms/tetris.gb\n");
    printf("\n");
}

static void
gb_args_parse(int argc, char **argv)
{
    int opt;

    while ((opt = getopt(argc, argv, "hd")) != -1) {
        switch (opt) {
        case 'd':
            gb_opts.debug = true;
            break;
        case 'h':
            gb_print_help();
            exit(0);
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

int
main(int argc, char **argv)
{
    gb_mapper_t mapper = {0};
    gb_rom_t rom = {0};
    gb_bus_t bus = {0};
    gb_cpu_t cpu = {0};
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

    while (1) {
        if (cpu.stall == 0 && gb_opts.debug) {
            gb_disasm_step(&bus, &cpu, stdout);
        }

        gb_cpu_step(&cpu, &bus);
        gb_bus_step(&bus);
        //usleep(1000);
    }

cleanup:
    gb_mapper_free(&mapper);
    gb_rom_free(&rom);

    return ret;
}
