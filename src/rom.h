#pragma once

#include <stdint.h>

#include "common.h"

#define ROM_MIN_SIZE (0x0100 + sizeof(ROMHeader))
#define ROM_MAX_SIZE (2 * 1024 * 1024) // 2MB
#define ROM_TITLE_SIZE 16

#define ROM_BANK_SIZE 0x4000
#define RAM_BANK_SIZE 0x2000

enum ROMType {
    ROM_TYPE_ROM_ONLY = 0x00,
    ROM_TYPE_MBC1 = 0x01,
    ROM_TYPE_MBC1_RAM = 0x02,
    ROM_TYPE_MBC1_RAM_BATT = 0x03,
};

typedef struct {
    uint8_t entry_point[4];     // 0x0100 - 0x0103
    uint8_t logo[48];           // 0x0104 - 0x0133
    char title[16];             // 0x0134 - 0x0143
    uint16_t new_licensee_code; // 0x0144 - 0x0145
    uint8_t sgb_flag;           // 0x0146
    uint8_t type;               // 0x0147
    uint8_t rom_size;           // 0x0148
    uint8_t ram_size;           // 0x0149
    uint8_t destination_code;   // 0x014A
    uint8_t old_licensee_code;  // 0x014B
    uint8_t mask_rom_version;   // 0x014C
    uint8_t header_checksum;    // 0x014D
    uint16_t global_checksum;   // 0x014E - 0x014F
} _packed_ ROMHeader;

typedef struct {
    ROMHeader *header;
    uint32_t size;
    uint8_t *data;
    uint32_t rom_size;
    uint32_t ram_size;
} ROM;

ROM *rom_open(const char *filename);

void rom_free(ROM **rom);
