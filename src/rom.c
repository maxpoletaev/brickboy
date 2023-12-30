#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#include "shared.h"
#include "rom.h"

int
gb_rom_open(gb_rom_t *rom, const char *filename)
{
    FILE *file = NULL;
    uint8_t *data = NULL;
    struct stat file_info = {0};
    int ret = GB_OK;

    file = fopen(filename, "rb");
    if (file == NULL) {
        GB_TRACE("failed to open file: %s", filename);
        ret = GB_ERR;
        goto cleanup;
    }

    if (stat(filename, &file_info) != 0) {
        GB_TRACE("failed to stat file: %s", filename);
        ret = GB_ERR;
        goto cleanup;
    }

    uint32_t rom_size = (uint32_t) file_info.st_size;
    if (rom_size < GB_ROM_MIN_SIZE || rom_size > GB_ROM_MAX_SIZE) {
        GB_TRACE("invalid rom size: %d", rom_size);
        ret = GB_ERR;
        goto cleanup;
    }

    data = calloc(1, rom_size);
    if (data == NULL) {
        GB_TRACE("failed to allocate rom data (%d bytes)", rom_size);
        ret = GB_ERR;
        goto cleanup;
    }

    if (fread(data, rom_size, 1, file) != 1) {
        GB_TRACE("failed to read rom data");
        ret = GB_ERR;
        goto cleanup;
    }

    // Header is mapped directly into the rom data at 0x0100.
    rom->header = (gb_romheader_t *) (data + 0x0100);
    rom->size = rom_size;
    rom->data = data;

    return ret;

cleanup:

    if (file != NULL) {
        fclose(file);
        file = NULL;
    }

    if (data != NULL) {
        free(data);
        data = NULL;
    }

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
