#ifndef BRICKBOY_SHARED_H
#define BRICKBOY_SHARED_H

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef BUILD_COMMIT_HASH
#define BUILD_COMMIT_HASH "unknown"
#endif

#ifndef BUILD_VERSION
#define BUILD_VERSION "unknown"
#endif

#define RET_OK 0

#define RET_ERR 1

#define UNUSED(x) (void)(x)

#define LOG(fmt, ...) fprintf(stdout, fmt "\n", ##__VA_ARGS__)

#define TRACE(fmt, ...) fprintf(stderr, fmt " (%s:%d)\n", ##__VA_ARGS__, __func__, __LINE__)

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define PANIC(fmt, ...) \
    do { \
        fprintf(stderr, "panic: " fmt " (%s:%d)\n", ##__VA_ARGS__, __func__, __LINE__); \
        abort(); \
    } while (0)

char *to_cstring(char *data, size_t size);

#endif //BRICKBOY_SHARED_H
