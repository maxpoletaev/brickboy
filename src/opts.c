#include <stdbool.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "opts.h"

static void
opts_usage(void)
{
    printf("Usage:\n");
    printf("  brickboy [OPTIONS...] romfile.gb \n");
}

static void
opts_help(void)
{
    printf("BrickBoy\n");
    printf("\n");

    opts_usage();
    printf("\n");

    printf("Options:\n");
    printf("  -h, --help           Print this help message\n");
    printf("\n");

    printf("Debug Options:\n");
    printf("  -d, --debug <debug_out>  Enable debug mode (disassemble each instruction before executing it)\n");
    printf("  -l, --state <state_out>  Enable state log mode (log CPU state after each instruction)\n");
    printf("  --slow                   Slow down the emulation speed\n");
}

static const struct option opts_long[] = {
    {"help", no_argument, NULL, 'h'},

    {"debug", required_argument, NULL, 'd'},
    {"state", required_argument, NULL, 'l'},
    {"breakpoint", required_argument, NULL, 'b'},
    {"debug-serial", no_argument, NULL, 0},
    {"nologo", no_argument, NULL, 0},
    {"slow", no_argument, NULL, 0},

    {NULL, 0, NULL, 0},
};

void
opts_parse(Opts *opts, int argc, char **argv)
{
    while (1) {
        int opt_index = 0;
        int opt = getopt_long(argc, argv, "hd:l:b:",
                              opts_long, &opt_index);

        if (opt == -1) {
            break;
        }

        if (opt == 0) {
            const struct option *o = &opts_long[opt_index];
            const char *name = o->name;

            if (strcmp(name, "slow") == 0) {
                opts->slow = true;
            } else if (strcmp(name, "nologo") == 0) {
                opts->no_logo = true;
            }

            continue;
        }

        switch (opt) {
        case 'd':
            opts->debug_out = optarg;
            break;
        case 'l':
            opts->state_out = optarg;
            break;
        case 'h':
            opts_help();
            exit(0);
        default:
            opts_usage();
            exit(1);
        }
    }

    if (optind >= argc) {
        opts_usage();
        exit(1);
    }

    opts->romfile = argv[optind];
}
