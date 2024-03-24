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
    uint8_t *data = xalloc(file_size);

    if (fread(data, file_size, 1, file) != 1) {
        TRACE("failed to read rom data");
        xfree(data);
        return RET_ERR;
    }

    BOUNDS_CHECK(file_size, 0x0100 + sizeof(ROMHeader));
    rom->header = (ROMHeader *) (data + 0x0100);

    rom->size = file_size;
    rom->data = data;

    return RET_OK;
}

ROM *
rom_open(const char *filename)
{
    ROM *rom = NULL;
    struct stat file_info = {0};
    _autoclose_ FILE *file = fopen(filename, "rb");

    if (file == NULL) {
        TRACE("failed to open file: %s", filename);
        return NULL;
    }

    if (stat(filename, &file_info) != 0) {
        TRACE("failed to STAT file: %s", filename);
        return NULL;
    }

    rom = xalloc(sizeof(ROM));
    uint32_t file_size = file_info.st_size;

    if (rom_load(rom, file, file_size) != RET_OK) {
        TRACE("failed to load rom: %s", filename);
        rom_free(&rom);
        return NULL;
    }

    switch (rom->header->ram_size) {
    case 0x00: rom->ram_size = 0; break;
    case 0x01: rom->ram_size = 2*1024; break;
    case 0x02: rom->ram_size = 8*1024; break;
    case 0x03: rom->ram_size = 32*1024; break;
    case 0x04: rom->ram_size = 128*1024; break;
    case 0x05: rom->ram_size = 64*1024; break;
    default: PANIC("invalid ram size: %02X", rom->header->ram_size);
    }

    char title[ROM_TITLE_SIZE + 1] = {0};
    strncpy(title, rom->header->title, ROM_TITLE_SIZE);

    LOG("ROM loaded:");
    LOG("  Title: %s", title);
    LOG("  Type: 0x%02X", rom->header->type);
    LOG("  RAM Size: %dKB", rom->ram_size / 1024);
    LOG("  ROM Size: %dKB", rom->size / 1024);

    return rom;
}

void
rom_free(ROM **rom)
{
    if (*rom != NULL) {
        (*rom)->header = NULL;
        xfree((*rom)->data);
        xfree(*rom);
    }
}
