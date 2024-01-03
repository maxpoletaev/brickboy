#ifndef BRICKBOY_SHARED_H
#define BRICKBOY_SHARED_H

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#define GB_OK 0

#define GB_ERR 1

#define GB_UNUSED(x) (void)(x)

#define GB_LOG(fmt, ...) fprintf(stdout, fmt "\n", ##__VA_ARGS__)

#define GB_TRACE(fmt, ...) fprintf(stderr, "(%s:%d): " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)

#define GB_ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define GB_PANIC(fmt, ...) \
    do { \
        fprintf(stderr, "panic: " fmt " (%s:%d)\n", ##__VA_ARGS__, __func__, __LINE__); \
        abort(); \
    } while (0)

#define GB_BOUNDS_CHECK(arr, idx)                                                   \
    do {                                                                            \
        if ((idx) >= GB_ARRAY_SIZE(arr)) {                                          \
            GB_PANIC("index out of bounds"); \
        }                                                                           \
    } while (0)

char *gb_cstring(char *data, size_t size);

#endif //BRICKBOY_SHARED_H
