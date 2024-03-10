#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct Opts {
    char *romfile;
    char *debug_out;
    char *state_out;
    bool no_logo;
    bool slow;
} Opts;

void opts_parse(Opts *opts, int argc, char **argv);
