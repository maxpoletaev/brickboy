#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "mapper.h"
#include "disasm.h"
#include "serial.h"
#include "timer.h"
#include "mmu.h"
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
    printf("BrickBoy\n");
    printf("\n");

    print_usage();
    printf("\n");

    printf("Options:\n");
    printf("  -h, --help           Print this help message\n");
    printf("\n");

    printf("Debug Options:\n");
    printf("  -d, --debug <debug_out>  Enable debug mode (disassemble each instruction before executing it)\n");
    printf("  -l, --state <state_out>  Enable state log mode (log CPU state after each instruction)\n");
    printf("  --breakpoint <AAAA>      Set breakpoint to $AAAA. Exit when PC reaches this address\n");
    printf("  --debug-serial           Print serial output to stdout\n");
    printf("  --slow                   Slow down the emulation speed\n");
}

static const struct option longopts[] = {
    {"help", no_argument, NULL, 'h'},

    {"debug", required_argument, NULL, 'd'},
    {"state", required_argument, NULL, 'l'},
    {"breakpoint", required_argument, NULL, 'b'},
    {"debug-serial", no_argument, NULL, 0},
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

            if (strcmp(name, "debug-serial") == 0) {
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
    if (strcmp(filename, "-") == 0) {
        return stdout;
    }

    return fopen(filename, "w");
}


static inline void
print_state(CPU *cpu, MMU *bus, FILE *out)
{
    fprintf(out, "A: %02X F: %02X B: %02X C: %02X D: %02X E: %02X H: %02X L: %02X SP: %04X PC: 00:%04X",
            cpu->a, cpu->f, cpu->b, cpu->c, cpu->d, cpu->e, cpu->h, cpu->l, cpu->sp, cpu->pc);

    uint8_t bytes[4] = {
        mmu_read(bus, cpu->pc),
        mmu_read(bus, cpu->pc + 1),
        mmu_read(bus, cpu->pc + 2),
        mmu_read(bus, cpu->pc + 3),
    };

    fprintf(out, " (%02X %02X %02X %02X)\n", bytes[0], bytes[1], bytes[2], bytes[3]);
}

static bool inline
handle_interrupts(MMU *bus, CPU *cpu)
{
    static const uint8_t mask[] =  {INT_VBLANK, INT_LCD_STAT, INT_TIMER, INT_SERIAL, INT_JOYPAD};
    static const uint16_t addr[] = {0x0040, 0x0048, 0x0050, 0x0058, 0x0060};
    static_assert(ARRAY_SIZE(mask) == ARRAY_SIZE(addr), "");

    if (bus->intf == 0 || bus->inte == 0) {
        return false;
    }

    for (size_t i = 0; i < ARRAY_SIZE(mask); i++) {
        if ((bus->intf & mask[i]) && (bus->inte & mask[i])) {
            if (!cpu_interrupt(cpu, bus, addr[i])) {
                return false;
            }

            bus->intf &= ~mask[i];
            return true;
        }
    }

    return false;
}

int
main(int argc, char **argv)
{
    args_parse(argc, argv);

    if (!opts.no_logo) {
        print_logo();
    }

    int exit_code = 0;
    FILE *debug_out = NULL;
    FILE *state_out = NULL;


    if (opts.debug_out != NULL) {
        debug_out = output_file(opts.debug_out);

        if (debug_out == NULL) {
            LOG("failed to open debug output file: %s", opts.debug_out);
            exit(1);
        }
    }

    if (opts.state_out != NULL) {
        state_out = output_file(opts.state_out);

        if (state_out == NULL) {
            LOG("failed to open state output file: %s", opts.state_out);
            exit(1);
        }
    }

    if (access(opts.romfile, R_OK) != 0) { // NOLINT(misc-include-cleaner): R_OK is defined in unistd.h
        LOG("file not found: %s", opts.romfile);
        exit(1);
    }

    ROM rom = {0};
    if (rom_open(&rom, opts.romfile) != RET_OK) {
        LOG("failed to open rom file");
        exit(1);
    }

    char *title = to_cstring(rom.header->title, sizeof(rom.header->title));
    LOG("rom loaded: %s", title);
    free(title);

    Mapper mapper = {0};
    if (mapper_init(&mapper, &rom) != RET_OK) {
        LOG("failed to initialize mapper");
        exit(1);
    }

    MMU mmu = {0};
    mmu.mapper = &mapper;
    mmu_reset(&mmu);

    CPU cpu = {0};
    cpu_reset(&cpu);

    if (opts.print_serial) {
        LOG("printing serial output is enabled");
    }

    if (opts.breakpoint != 0) {
        LOG("breakpoint set to 0x%04X", opts.breakpoint);
    }

    while (1) {
        if (debug_out != NULL) {
            disasm_step(&mmu, &cpu, debug_out);
        }

        if (state_out != NULL) {
            print_state(&cpu, &mmu, state_out);
        }

        int cycles = cpu_step_fast(&cpu, &mmu);

        timer_step_fast(&mmu.timer, cycles);

        if (mmu.timer.interrupt) {
            mmu.intf |= INT_TIMER;
            mmu.timer.interrupt = false;
        }

        serial_print(&mmu.serial);
        handle_interrupts(&mmu, &cpu);

        if (opts.breakpoint && cpu.pc == opts.breakpoint) {
            LOG("breakpoint reached at 0x%04X", opts.breakpoint);
            break;
        }
    }

    mapper_free(&mapper);

    rom_free(&rom);

    if (debug_out != NULL && debug_out != stdout) {
        fclose(debug_out);
    }

    if (state_out != NULL && state_out != stdout) {
        fclose(state_out);
    }

    return exit_code;
}
