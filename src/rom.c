#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#include "shared.h"
#include "rom.h"


static int
gb_rom_load(gb_rom_t *rom, FILE *file, uint32_t file_size)
{
    if (file_size < GB_ROM_MIN_SIZE || file_size > GB_ROM_MAX_SIZE) {
        GB_TRACE("invalid rom size: %d", file_size);
        return GB_ERR;
    }

    uint8_t *data = calloc(1, file_size);
    if (data == NULL) {
        GB_TRACE("failed to allocate rom data (%d bytes)", file_size);
        goto error;
    }

    if (fread(data, file_size, 1, file) != 1) {
        GB_TRACE("failed to read rom data");
        goto error;
    }

    // Header is mapped directly into the rom data at 0x0100.
    rom->header = (gb_romheader_t *) (data + 0x0100);
    rom->size = file_size;
    rom->data = data;

    return GB_OK;

error:
    free(data);
    return GB_ERR;
}

int
gb_rom_open(gb_rom_t *rom, const char *filename)
{
    FILE *file = NULL;
    int ret = GB_OK;

    file = fopen(filename, "rb");
    if (file == NULL) {
        GB_TRACE("failed to open file: %s", filename);
        return GB_ERR;
    }

    struct stat file_info;
    if (stat(filename, &file_info) != 0) {
        GB_TRACE("failed to stat file: %s", filename);
        ret = GB_ERR;
        goto cleanup;
    }

    uint32_t file_size = (uint32_t) file_info.st_size;
    if (gb_rom_load(rom, file, file_size) != GB_OK) {
        GB_TRACE("failed to load rom: %s", filename);
        ret = GB_ERR;
        goto cleanup;
    }

cleanup:
    fclose(file);
    file = NULL;

    return ret;
}

void
gb_rom_free(gb_rom_t *rom)
{
    if (rom->data == NULL)
        return;

    free(rom->data);
    rom->header = NULL;
    rom->data = NULL;
    rom->size = 0;
}
