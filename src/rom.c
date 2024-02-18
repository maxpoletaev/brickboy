#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#include "common.h"
#include "rom.h"

static int
rom_load(ROM *rom, FILE *file, uint32_t file_size)
{
    if (file_size < ROM_MIN_SIZE || file_size > ROM_MAX_SIZE) {
        TRACE("invalid rom size: %d", file_size);
        return RET_ERR;
    }

    uint8_t *data = must_alloc(file_size);
    if (fread(data, file_size, 1, file) != 1) {
        TRACE("failed to read rom data");
        free(data);
        return RET_ERR;
    }

    rom->header = (ROMHeader *) (data + 0x0100);
    rom->size = file_size;
    rom->data = data;

    return RET_OK;
}

int
rom_open(ROM *rom, const char *filename)
{
    FILE *file = NULL;
    int ret = RET_OK;

    file = fopen(filename, "rb");
    if (file == NULL) {
        TRACE("failed to open file: %s", filename);
        return RET_ERR;
    }

    struct stat file_info;
    if (stat(filename, &file_info) != 0) {
        TRACE("failed to stat file: %s", filename);
        ret = RET_ERR;
        goto cleanup;
    }

    uint32_t file_size = (uint32_t) file_info.st_size;
    if (rom_load(rom, file, file_size) != RET_OK) {
        TRACE("failed to load rom: %s", filename);
        ret = RET_ERR;
        goto cleanup;
    }

cleanup:

    fclose(file);
    file = NULL;
    return ret;
}

void
rom_free(ROM *rom)
{
    free(rom->data);
    rom->header = NULL;
    rom->data = NULL;
    rom->size = 0;
}
