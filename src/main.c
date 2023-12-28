#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "mapper.h"
#include "bus.h"
#include "cpu.h"
#include "rom.h"

int
main(int argc, char **argv)
{
    gb_rom_t *rom = NULL;
    gb_mapper_t *mapper = NULL;

    if (argc != 2) {
        GB_TRACE("usage: %s <rom>", argv[0]);
        goto error;
    }

    char *rom_filename = argv[1];
    if (access(rom_filename, R_OK) != 0) {
        GB_TRACE("file not found: %s", rom_filename);
        goto error;
    }

    rom = calloc(1, sizeof(gb_rom_t));
    if (rom == NULL) {
        GB_TRACE("failed to allocate rom memory");
        goto error;
    }

    if (gb_rom_open(rom, rom_filename) != 0) {
        GB_TRACE("failed to open rom file");
        goto error;
    }

    mapper = calloc(1, sizeof(gb_mapper_t));
    if (mapper == NULL) {
        GB_TRACE("failed to allocate mapper memory");
        goto error;
    }

    if (gb_mapper_init(mapper, rom) != 0) {
        GB_TRACE("failed to initialize mapper");
        goto error;
    }

    char *title = gb_cstring(rom->header->title, sizeof(rom->header->title));
    GB_LOG("ROM loaded: %s", title);
    free(title);

    gb_bus_t bus = {0};
    bus.mapper = mapper;
    gb_bus_reset(&bus);

    gb_cpu_t cpu = {0};
    gb_cpu_reset(&cpu);

    while (1) {
        gb_cpu_step(&cpu, &bus);
        sleep(1);
    }

    gb_mapper_free(mapper);
    free(mapper);

    gb_rom_free(rom);
    free(rom);

    return 0;


error:

    if (mapper != NULL) {
        gb_mapper_free(mapper);
        free(mapper);
    }

    if (rom != NULL) {
        gb_rom_free(rom);
        free(rom);
    }

    return 1;
}
