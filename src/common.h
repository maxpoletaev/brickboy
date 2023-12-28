#ifndef BRICKBOY_COMMON_H
#define BRICKBOY_COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define GB_OK 0

#define GB_ERR 1

#define GB_COLOR_RED "\x1b[31m"

#define GB_COLOR_RESET "\x1b[0m"

#define GB_LOG(fmt, ...) fprintf(stdout, fmt "\n", ##__VA_ARGS__)

#define GB_TRACE(fmt, ...) fprintf(stderr, GB_COLOR_RED "(%s:%d): " fmt GB_COLOR_RESET "\n", __func__, __LINE__, ##__VA_ARGS__); \

#define GB_UNUSED(x) (void)(x)

#define GB_UNREACHABLE() \
    do { \
        GB_TRACE("unreachable code"); \
        abort(); \
    } while (0)

static inline char *
gb_cstring(char *data, size_t size)
{
    char *str = malloc(size + 1);
    strncpy(str, data, size);
    str[size] = '\0';
    return str;
}

#endif //BRICKBOY_COMMON_H
