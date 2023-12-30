#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "shared.h"
#include "mapper.h"
#include "disasm.h"
#include "bus.h"
#include "cpu.h"
#include "rom.h"

int
main(int argc, char **argv)
{
    gb_mapper_t mapper = {0};
    gb_rom_t rom = {0};
    gb_bus_t bus = {0};
    gb_cpu_t cpu = {0};

    if (argc != 2) {
        GB_TRACE("usage: %s <rom>", argv[0]);
        return 1;
    }

    char *rom_filename = argv[1];

    if (access(rom_filename, R_OK) != 0) { // NOLINT(misc-include-cleaner): R_OK is defined in unistd.h
        GB_TRACE("file not found: %s", rom_filename);
        goto cleanup;
    }

    if (gb_rom_open(&rom, rom_filename) != GB_OK) {
        GB_TRACE("failed to open rom file");
        goto cleanup;
    }

    if (gb_mapper_init(&mapper, &rom) != GB_OK) {
        GB_TRACE("failed to initialize mapper");
        goto cleanup;
    }

    char *title = gb_cstring(rom.header->title, sizeof(rom.header->title));
    GB_LOG("rom loaded: %s (%s)", title, mapper.name);
    free(title);

    bus.cpu = &cpu;
    bus.mapper = &mapper;
    gb_bus_reset(&bus);

    while (1) {
        if (cpu.halt == 0) {
            gb_disasm_step(&bus, stdout);
        }

        gb_bus_step(&bus);
    }

    gb_mapper_free(&mapper);
    gb_rom_free(&rom);

    return 0;

cleanup:

    gb_mapper_free(&mapper);
    gb_rom_free(&rom);

    return 1;
}
