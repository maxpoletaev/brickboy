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

static struct Opts {
    char *romfile;
    char *debug_out;
    char *state_out;
    uint16_t breakpoint;
    bool print_serial;
    bool no_logo;
    bool slow;
} opts;

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

static void
print_usage(void)
{
    printf("Usage:\n");
    printf("  brickboy [OPTIONS...] romfile.gb \n");
}

static void
print_help(void)
{
    printf("BrickBoy %s (%s)\n", BUILD_VERSION, BUILD_COMMIT_HASH);
    printf("\n");

    print_usage();
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

static const struct option longopts[] = {
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
args_parse(int argc, char **argv)
{
    while (1) {
        int opt_index = 0;
        int opt = getopt_long(argc, argv, "hd:l:b:",
                              longopts, &opt_index);

        if (opt == -1)
            break;

        if (opt == 0) {
            const struct option *o = &longopts[opt_index];
            const char *name = o->name;

            if (strcmp(name, "slow") == 0) {
                opts.slow = true;
                continue;
            }

            if (strcmp(name, "serial") == 0) {
                opts.print_serial = true;
                continue;
            }

            if (strcmp(name, "breakpoint") == 0) {
                opts.breakpoint = (uint16_t) strtoul(optarg, NULL, 16);

                if (errno != 0) {
                    LOG("invalid breakpoint address: %s", optarg);
                    exit(1);
                }

                continue;
            }

            if (strcmp(name, "nologo") == 0) {
                opts.no_logo = true;
                continue;
            }

            continue;
        }

        switch (opt) {
        case 'd':
            opts.debug_out = optarg;
            break;
        case 'l':
            opts.state_out = optarg;
            break;
        case 's':
            opts.print_serial = true;
            break;
        case 'h':
            print_help();
            exit(0);
        case 'b':
            opts.breakpoint = (uint16_t) strtoul(optarg, NULL, 16);

            if (errno != 0) {
                LOG("invalid breakpoint address: %s", optarg);
                exit(1);
            }
            break;
        default:
            print_usage();
            exit(1);
        }
    }

    if (optind >= argc) {
        print_usage();
        exit(1);
    }

    opts.romfile = argv[optind];
}

static FILE*
output_file(const char *filename)
{
    if (strcmp(filename, "-") == 0)
        return stdout;

    return fopen(filename, "w");
}


static inline void
print_state(CPU *cpu, Bus *bus, FILE *out)
{
    fprintf(out, "A: %02X F: %02X B: %02X C: %02X D: %02X E: %02X H: %02X L: %02X SP: %04X PC: 00:%04X",
            cpu->a, cpu->f, cpu->b, cpu->c, cpu->d, cpu->e, cpu->h, cpu->l, cpu->sp, cpu->pc);

    uint8_t bytes[4] = {
        bus_read(bus, cpu->pc),
        bus_read(bus, cpu->pc + 1),
        bus_read(bus, cpu->pc + 2),
        bus_read(bus, cpu->pc + 3),
    };

    fprintf(out, " (%02X %02X %02X %02X)\n", bytes[0], bytes[1], bytes[2], bytes[3]);
}

#define MAIN_EXIT(code, fmt, ...) \
    do {                             \
        LOG(fmt, ##__VA_ARGS__);  \
        exit_code = code;            \
        goto cleanup;                \
    } while (0)

int
main(int argc, char **argv)
{
    args_parse(argc, argv);

    if (!opts.no_logo)
        print_logo();

    int exit_code = 0;
    FILE *debug_out = NULL;
    FILE *state_out = NULL;

    Mapper mapper;
    ROM rom;
    Bus bus;
    CPU cpu;

    if (opts.debug_out != NULL) {
        debug_out = output_file(opts.debug_out);
        if (debug_out == NULL)
            MAIN_EXIT(1, "failed to open debug output file: %s", opts.debug_out);
    }

    if (opts.state_out != NULL) {
        state_out = output_file(opts.state_out);

        if (state_out == NULL)
            MAIN_EXIT(1, "failed to open state output file: %s", opts.state_out);
    }

    if (access(opts.romfile, R_OK) != 0) // NOLINT(misc-include-cleaner): R_OK is defined in unistd.h
        MAIN_EXIT(1, "file not found: %s", opts.romfile);

    if (rom_open(&rom, opts.romfile) != RET_OK)
        MAIN_EXIT(1, "failed to open rom file");

    if (mapper_init(&mapper, &rom) != RET_OK)
        MAIN_EXIT(1, "failed to initialize mapper");

    char *title = to_cstring(rom.header->title, sizeof(rom.header->title));
    LOG("rom loaded: %s (%s)", title, mapper.name);
    free(title);

    bus.mapper = &mapper;
    bus_reset(&bus);
    cpu_reset(&cpu);

    if (opts.print_serial)
        LOG("printing serial output is enabled");

    if (opts.breakpoint != 0)
        LOG("breakpoint set to 0x%04X", opts.breakpoint);

    while (1) {
        if (cpu.remaining == 0) {
            if (debug_out != NULL)
                disasm_step(&bus, &cpu, debug_out);
            if (state_out != NULL)
                print_state(&cpu, &bus, state_out);

            if (bus.ie_bits.vblank && bus.if_bits.vblank) {
                cpu_interrupt(&cpu, &bus, INTERRUPT_VBLANK);
                bus.if_bits.vblank = 0;
            } else if (bus.ie_bits.lcd_stat && bus.if_bits.lcd_stat) {
                cpu_interrupt(&cpu, &bus, INTERRUPT_LCDSTAT);
                bus.if_bits.lcd_stat = 0;
            } else if (bus.ie_bits.timer && bus.if_bits.timer) {
                cpu_interrupt(&cpu, &bus, INTERRUPT_TIMER);
                bus.if_bits.timer = 0;
            } else if (bus.ie_bits.serial && bus.if_bits.serial) {
                cpu_interrupt(&cpu, &bus, INTERRUPT_SERIAL);
                bus.if_bits.serial = 0;
            } else if (bus.ie_bits.joypad && bus.if_bits.joypad) {
                cpu_interrupt(&cpu, &bus, INTERRUPT_JOYPAD);
                bus.if_bits.joypad = 0;
            }
        }

        cpu_step(&cpu, &bus);

        if (bus.sc == 0x81 && opts.print_serial) {
            fputc(bus.sb, stdout);
            bus.sc = 0x01;
            fflush(stdout);
        }

        if (opts.breakpoint > 0 && cpu.pc == opts.breakpoint) {
            LOG("breakpoint reached at 0x%04X\n", cpu.pc);
            break;
        }

        if (opts.slow)
            usleep(1000);
    }

cleanup:
    mapper_free(&mapper);
    rom_free(&rom);

    if (debug_out != NULL && debug_out != stdout)
        fclose(debug_out);
    if (state_out != NULL && state_out != stdout)
        fclose(state_out);


    return exit_code;
}
