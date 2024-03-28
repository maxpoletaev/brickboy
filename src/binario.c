#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "binario.h"

size_t
fwrite_u8(uint8_t data, FILE *file)
{
    return fwrite(&data, sizeof(uint8_t), 1, file);
}

size_t
fwrite_u16(uint16_t data, FILE *file)
{
    uint8_t buf[2] = {
        (data >> 0) & 0xFF,
        (data >> 8) & 0xFF,
    };

    return fwrite(buf, sizeof(uint8_t), 2, file);
}

size_t
fwrite_u32(uint32_t data, FILE *file)
{
    uint8_t buf[4];
    for (int i = 0; i < 4; i++) {
        buf[i] = (data >> (i * 8)) & 0xFF;
    }

    return fwrite(buf, sizeof(uint8_t), 4, file);
}

size_t
fwrite_u64(uint64_t data, FILE *file)
{
    uint8_t buf[8];
    for (int i = 0; i < 4; i++) {
        buf[i] = (data >> (i * 8)) & 0xFF;
    }

    return fwrite(buf, sizeof(uint8_t), 8, file);
}

size_t
fread_u8(uint8_t *data, FILE *file)
{
    return fread(data, sizeof(uint8_t), 1, file);
}

size_t
fread_u16(uint16_t *data, FILE *file)
{
    uint8_t buf[2];
    size_t ret = fread(buf, sizeof(uint8_t), 2, file);
    *data = (buf[0] << 0) | (buf[1] << 8);

    return ret;
}

size_t
fread_u32(uint32_t *data, FILE *file)
{
    uint8_t buf[4];
    size_t ret = fread(buf, sizeof(uint8_t), 4, file);

    for (int i = 0; i < 4; i++) {
        *data |= (uint32_t) buf[i] << (i * 8);
    }

    return ret;
}

size_t
fread_u64(uint64_t *data, FILE *file)
{
    uint8_t buf[8] = {0};
    size_t ret = fread(buf, sizeof(uint8_t), 8, file);

    for (int i = 0; i < 8; i++) {
        *data |= (uint64_t) buf[i] << (i * 8);
    }

    return ret;
}
