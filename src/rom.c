#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/_types/_null.h>
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

ROM *
rom_open(const char *filename)
{
    ROM *rom = NULL;
    FILE *file _autoclose_ = NULL;

    file = fopen(filename, "rb");
    if (file == NULL) {
        TRACE("failed to open file: %s", filename);
        goto failure;
    }

    struct stat file_info;
    if (stat(filename, &file_info) != 0) {
        TRACE("failed to STAT file: %s", filename);
        goto failure;
    }

    rom = xalloc(sizeof(ROM));
    uint32_t file_size = (uint32_t) file_info.st_size;
    if (rom_load(rom, file, file_size) != RET_OK) {
        TRACE("failed to load rom: %s", filename);
        goto failure;
    }

    const char *title = rom_title(rom);
    LOG("rom loaded: %s", title);

    return rom;

failure:
    xfree(rom);
    return NULL;
}

void
rom_free(ROM **rom)
{
    (*rom)->header = NULL;
    xfree((*rom)->data);
    xfree(*rom);
}
