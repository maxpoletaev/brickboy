#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>

#include "shared.h"
#include "mapper.h"
#include "disasm.h"
#include "bus.h"
#include "cpu.h"
#include "rom.h"

static struct gb_opts_t {
    char *romfile;
    char *debug_out;
    char *state_out;
    uint16_t breakpoint;
    bool print_serial;
    bool no_logo;
    bool slow;
} gb_opts;

static void
gb_print_logo(void)
{
    printf(" _          _      _    _                      \n");
    printf("| |__  _ __(_) ___| | _| |__   ___  _   _      \n");
    printf("| '_ \\| '__| |/ __| |/ / '_ \\ / _ \\| | | |  \n");
    printf("| |_) | |  | | (__|   <| |_) | (_) | |_| |     \n");
    printf("|_.__/|_|  |_|\\___|_|\\_\\_.__/ \\___/ \\__, |\n");
    printf("                                    |___/      \n");
}

static void
gb_print_usage(void)
{
    printf("Usage:\n");
    printf("  brickboy [OPTIONS...] romfile.gb \n");
}

static void
gb_print_help(void)
{
    printf("BrickBoy %s (%s)\n", BUILD_VERSION, BUILD_COMMIT_HASH);
    printf("\n");

    gb_print_usage();
    printf("\n");

    printf("Options:\n");
    printf("  -h, --help           Print this help message\n");
    printf("\n");

    printf("Debug Options:\n");
    printf("  -d, --debug <romfile>    Enable debug mode (disassemble each instruction before executing it)\n");
    printf("  -l, --state <romfile>    Enable state log mode (log CPU state after each instruction)\n");
    printf("  --breakpoint <AAAA>      Set breakpoint to $AAAA. Exit when PC reaches this address\n");
    printf("  --print-serial           Print serial output to stdout\n");
    printf("  --slow                   Slow down the emulation speed\n");
}

static const struct option gb_longopts[] = {
    {"help", no_argument, NULL, 'h'},

    {"debuglog", required_argument, NULL, 'd'},
    {"statelog", required_argument, NULL, 'l'},
    {"breakpoint", required_argument, NULL, 'b'},
    {"serial", no_argument, NULL, 0},
    {"nologo", no_argument, NULL, 0},
    {"slow", no_argument, NULL, 0},

    {NULL, 0, NULL, 0},
};

static void
gb_args_parse(int argc, char **argv)
{
    while (1) {
        int opt_index = 0;
        int opt = getopt_long(argc, argv, "hd:l:b:",
                              gb_longopts, &opt_index);

        if (opt == -1)
            break;

        if (opt == 0) {
            const struct option *o = &gb_longopts[opt_index];
            const char *name = o->name;

            if (strcmp(name, "slow") == 0) {
                gb_opts.slow = true;
                continue;
            }

            if (strcmp(name, "serial") == 0) {
                gb_opts.print_serial = true;
                continue;
            }

            if (strcmp(name, "breakpoint") == 0) {
                gb_opts.breakpoint = (uint16_t) strtoul(optarg, NULL, 16);

                if (errno != 0) {
                    GB_LOG("invalid breakpoint address: %s", optarg);
                    exit(1);
                }

                continue;
            }

            if (strcmp(name, "nologo") == 0) {
                gb_opts.no_logo = true;
                continue;
            }

            continue;
        }

        switch (opt) {
        case 'd':
            gb_opts.debug_out = optarg;
            break;
        case 'l':
            gb_opts.state_out = optarg;
            break;
        case 's':
            gb_opts.print_serial = true;
            break;
        case 'h':
            gb_print_help();
            exit(0);
        case 'b':
            gb_opts.breakpoint = (uint16_t) strtoul(optarg, NULL, 16);

            if (errno != 0) {
                GB_LOG("invalid breakpoint address: %s", optarg);
                exit(1);
            }
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

    gb_opts.romfile = argv[optind];
}

static FILE*
gb_output_file(const char *filename)
{
    if (strcmp(filename, "-") == 0)
        return stdout;

    return fopen(filename, "w");
}


static inline void
gb_print_state(gb_cpu_t *cpu, gb_bus_t *bus, FILE *out)
{
    fprintf(out, "A: %02X F: %02X B: %02X C: %02X D: %02X E: %02X H: %02X L: %02X SP: %04X PC: 00:%04X",
            cpu->a, cpu->f, cpu->b, cpu->c, cpu->d, cpu->e, cpu->h, cpu->l, cpu->sp, cpu->pc);

    uint8_t bytes[4] = {
        gb_bus_read(bus, cpu->pc),
        gb_bus_read(bus, cpu->pc + 1),
        gb_bus_read(bus, cpu->pc + 2),
        gb_bus_read(bus, cpu->pc + 3),
    };

    fprintf(out, " (%02X %02X %02X %02X)\n", bytes[0], bytes[1], bytes[2], bytes[3]);
}

#define GB_MAIN_EXIT(code, fmt, ...) \
    do {                             \
                                     \
        GB_LOG(fmt, ##__VA_ARGS__);  \
        exit_code = code;            \
        goto cleanup;                \
    } while (0)

int
main(int argc, char **argv)
{
    gb_args_parse(argc, argv);

    if (!gb_opts.no_logo)
        gb_print_logo();

    int exit_code = 0;
    FILE *debug_out = NULL;
    FILE *state_out = NULL;
    gb_mapper_t mapper;
    gb_rom_t rom;
    gb_bus_t bus;
    gb_cpu_t cpu;

    if (gb_opts.debug_out != NULL) {
        debug_out = gb_output_file(gb_opts.debug_out);
        if (debug_out == NULL)
            GB_MAIN_EXIT(1, "failed to open debug output file: %s", gb_opts.debug_out);
    }

    if (gb_opts.state_out != NULL) {
        state_out = gb_output_file(gb_opts.state_out);

        if (state_out == NULL)
            GB_MAIN_EXIT(1, "failed to open state output file: %s", gb_opts.state_out);
    }

    if (access(gb_opts.romfile, R_OK) != 0) // NOLINT(misc-include-cleaner): R_OK is defined in unistd.h
        GB_MAIN_EXIT(1, "file not found: %s", gb_opts.romfile);

    if (gb_rom_open(&rom, gb_opts.romfile) != GB_OK)
        GB_MAIN_EXIT(1, "failed to open rom file");

    if (gb_mapper_init(&mapper, &rom) != GB_OK)
        GB_MAIN_EXIT(1, "failed to initialize mapper");

    char *title = gb_cstring(rom.header->title, sizeof(rom.header->title));
    GB_LOG("rom loaded: %s (%s)", title, mapper.name);
    free(title);

    bus.mapper = &mapper;
    gb_bus_reset(&bus);
    gb_cpu_reset(&cpu);

    if (gb_opts.print_serial)
        GB_LOG("printing serial output is enabled");

    if (gb_opts.breakpoint != 0)
        GB_LOG("breakpoint set to 0x%04X", gb_opts.breakpoint);

    while (1) {
        bool end_of_instruction = cpu.stall == 0;

        if (end_of_instruction) {
            if (debug_out != NULL)
                gb_disasm_step(&bus, &cpu, debug_out);
            if (state_out != NULL)
                gb_print_state(&cpu, &bus, state_out);
        }

        gb_cpu_step(&cpu, &bus);
        gb_bus_step(&bus);

        if (bus.serial_ctrl == 0x81 && gb_opts.print_serial) {
            fputc(bus.serial_data, stdout);
            bus.serial_ctrl = 0x01;
            fflush(stdout);
        }

        if (gb_opts.breakpoint > 0 && cpu.pc == gb_opts.breakpoint) {
            GB_LOG("breakpoint reached at 0x%04X\n", cpu.pc);
            break;
        }

        if (gb_opts.slow)
            usleep(1000);
    }

cleanup:
    gb_mapper_free(&mapper);
    gb_rom_free(&rom);

    if (debug_out != NULL && debug_out != stdout)
        fclose(debug_out);
    if (state_out != NULL && state_out != stdout)
        fclose(state_out);


    return exit_code;
}
