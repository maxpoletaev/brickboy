#ifndef BRICKBOY_LOG_H
#define BRICKBOY_LOG_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define GB_OK 0;

#define GB_ERR 1;

#define GB_LOG(fmt, ...) fprintf(stdout, fmt "\n", ##__VA_ARGS__)

#define GB_TRACE(fmt, ...) fprintf(stderr, "(%s:%d): " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)

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

#endif //BRICKBOY_LOG_H
