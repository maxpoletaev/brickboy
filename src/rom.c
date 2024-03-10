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

    uint8_t *data = xalloc(file_size);
    if (fread(data, file_size, 1, file) != 1) {
        TRACE("failed to read rom data");
        xfree(data);
        return RET_ERR;
    }

    rom->header = (ROMHeader *) (data + 0x0100);
    rom->size = file_size;
    rom->data = data;

    return RET_OK;
}

static const char *
rom_title(ROM *rom)
{
    static char title[ROM_TITLE_SIZE + 1] = {0};
    strncpy(title, (char *) rom->header->title, ROM_TITLE_SIZE);
    title[ROM_TITLE_SIZE] = '\0';
    return title;
}

int
rom_init(ROM *rom, const char *filename)
{
    FILE *file _autoclose_ = fopen(filename, "rb");
    if (file == NULL) {
        TRACE("failed to open file: %s", filename);
        return RET_ERR;
    }

    struct stat file_info;
    if (stat(filename, &file_info) != 0) {
        TRACE("failed to STAT file: %s", filename);
        return RET_ERR;
    }

    uint32_t file_size = (uint32_t) file_info.st_size;
    if (rom_load(rom, file, file_size) != RET_OK) {
        TRACE("failed to load rom: %s", filename);
        return RET_ERR;
    }

    const char *title = rom_title(rom);
    LOG("rom loaded: %s", title);

    return RET_OK;
}

void
rom_deinit(ROM *rom)
{
    xfree(rom->data);
}
