#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#include "common.h"
#include "rom.h"

int
gb_rom_open(gb_rom_t *rom, char *filename)
{
    uint8_t *data = NULL;
    FILE *file = NULL;
    struct stat file_info;

    file = fopen(filename, "rb");
    if (file == NULL) {
        GB_TRACE("failed to open file: %s", filename);
        goto error;
    }

    if (stat(filename, &file_info) != 0) {
        GB_TRACE("failed to stat file: %s", filename);
        goto error;
    }

    uint32_t rom_size = file_info.st_size;

    if (rom_size < GB_ROM_MIN_SIZE || rom_size > GB_ROM_MAX_SIZE) {
        GB_TRACE("invalid rom size: %d", rom_size);
        goto error;
    }

    data = malloc(rom_size);
    if (data == NULL) {
        GB_TRACE("failed to allocate rom data (%d bytes)", rom_size);
        goto error;
    }

    if (fread(data, rom_size, 1, file) != 1) {
        GB_TRACE("failed to read rom data");
        goto error;
    }

    rom->header = (gb_romheader_t *) (data + 0x0100);
    rom->size = rom_size;
    rom->data = data;

    return 0;

error:

    if (file != NULL) {
        fclose(file);
        file = NULL;
    }

    if (data != NULL) {
        free(data);
        data = NULL;
    }

    return 1;
}

void
gb_rom_free(gb_rom_t *rom)
{
    free(rom->data);
    rom->header = NULL;
    rom->data = NULL;
    rom->size = 0;
}
