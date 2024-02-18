#ifndef BRICKBOY_COMMON_H
#define BRICKBOY_COMMON_H

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#define RET_OK 0

#define RET_ERR 1

#define UNUSED(x) (void)(x)

#define LOG(fmt, ...) fprintf(stdout, fmt "\n", ##__VA_ARGS__)

#define TRACE(fmt, ...) fprintf(stderr, fmt " (%s:%d)\n", ##__VA_ARGS__, __func__, __LINE__)

#define PACKED __attribute__((packed))

#define ALIGNED(x) __attribute__((aligned(x)))

#define SET_BIT(x, n) ((x) |= (1 << (n)))

#define CLEAR_BIT(x, n) ((x) &= ~(1 << (n)))

#define TEST_BIT(x, n) ((x) & (1 << (n)))

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define PANIC(fmt, ...) \
    do { \
        fprintf(stderr, "panic: " fmt " (%s:%d)\n", ##__VA_ARGS__, __func__, __LINE__); \
        abort(); \
    } while (0)

/* Returns a string copy that is guaranteed to be null-terminated. */
char *to_cstring(const char *data, size_t size);

/* Allocates memory or panics if allocation fails. */
void *must_alloc(size_t size);

#endif //BRICKBOY_COMMON_H
