#ifndef BRICKBOY_ROM_H
#define BRICKBOY_ROM_H

#define GB_ROM_MIN_SIZE 0x0100 + sizeof(gb_romheader_t)
#define GB_ROM_MAX_SIZE (2*1024*1024) // 2MB

#include <stdint.h>

typedef struct gb_romheader_t {
    uint8_t entry_point[4];     // 0x0100 - 0x0103
    uint8_t logo[48];           // 0x0104 - 0x0133
    char title[16];             // 0x0134 - 0x0143
    uint16_t new_licensee_code; // 0x0144 - 0x0145
    uint8_t sgb_flag;           // 0x0146
    uint8_t cartridge_type;     // 0x0147
    uint8_t rom_size;           // 0x0148
    uint8_t ram_size;           // 0x0149
    uint8_t destination_code;   // 0x014A
    uint8_t old_licensee_code;  // 0x014B
    uint8_t mask_rom_version;   // 0x014C
    uint8_t header_checksum;    // 0x014D
    uint16_t global_checksum;   // 0x014E - 0x014F
} gb_romheader_t;

typedef struct {
    gb_romheader_t *header;
    uint8_t *data;
    uint32_t size;
} gb_rom_t;

int gb_rom_open(gb_rom_t *rom, const char *filename);

void gb_rom_free(gb_rom_t *rom);

#endif //BRICKBOY_ROM_H
