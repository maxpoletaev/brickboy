#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#define RET_OK 0

#define RET_ERR 1

#define UNUSED(x) (void)(x)

#define LOG(fmt, ...) fprintf(stdout, fmt "\n", ##__VA_ARGS__)

#define TRACE(fmt, ...) fprintf(stderr, fmt " (%s:%d)\n", ##__VA_ARGS__, __func__, __LINE__)

#define CONTAINER_OF(ptr, type, member) ((type *) ((char *) (ptr) - offsetof(type, member)))

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define SETBIT(x, n) ((x) |= (1 << (n)))

#define CLRBIT(x, n) ((x) &= ~(1 << (n)))

#define GETBIT(x, n) (((x) >> (n)) & 1)

#define PANIC(fmt, ...) \
    do { \
        fprintf(stderr, "panic: " fmt " (%s:%d)\n", ##__VA_ARGS__, __func__, __LINE__); \
        exit(3); \
    } while (0)

// Format a string intended for temporary use (e.g. logging).
// Returns a pointer to a static buffer that is overwritten on each call.
const char *strfmt(const char *fmt, ...);

// Allocates a zeroed block of memory and panics if allocation fails.
void *xalloc(size_t size);

// Reallocates a block of memory and panics if reallocation fails.
void *xrealloc(void *ptr, size_t old_size, size_t new_size);

// Frees a pointer and sets it to NULL.
#define xfree(ptr) free(ptr); (ptr) = NULL;

// Cleanup function for pointers allocated with xalloc.
void free_ptr(void *p);

// Cleanup function for FILE pointers.
void fclose_ptr(FILE **p);

// RAII-style cleanup for arbitrary resources.
#define _cleanup_(x) __attribute__((cleanup(x)))

// RAII-style cleanup attribute for pointers allocated with malloc.
#define _autofree_ __attribute__((cleanup(free_ptr)))

// RAII-style cleanup attribute for FILE pointers.
#define _autoclose_ __attribute__((cleanup(fclose_ptr)))

// Pack struct members without padding.
#define _packed_ __attribute__((packed))

// Align struct members to a specific byte boundary.
#define _aligned_(x) __attribute__((aligned(x)))

// Mark a function as unused to suppress warnings.
#define _unused_ __attribute__((unused))

// Fall through in a switch statement (for use with -Wimplicit-fallthrough).
#define _fallthrough_ __attribute__((fallthrough))
