#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

size_t fwrite_u8(uint8_t data, FILE *file);

size_t fwrite_u16(uint16_t data, FILE *file);

size_t fwrite_u32(uint32_t data, FILE *file);

size_t fwrite_u64(uint64_t data, FILE *file);

size_t fread_u8(uint8_t *data, FILE *file);

size_t fread_u16(uint16_t *data, FILE *file);

size_t fread_u32(uint32_t *data, FILE *file);

size_t fread_u64(uint64_t *data, FILE *file);
