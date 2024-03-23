#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "common.h"

inline void *
xalloc(size_t size)
{
    void *ptr = calloc(1, size);

    if (ptr == NULL) {
        PANIC("failed to allocate %zu bytes", size);
    }

    return ptr;
}

inline void *
xrealloc(void *ptr, size_t old_size, size_t new_size)
{
    void *new_ptr = realloc(ptr, new_size);

    if (new_ptr == NULL) {
        PANIC("failed to reallocate %zu bytes", new_size);
    }

    if (new_size > old_size) {
        memset((char *) new_ptr + old_size, 0, new_size - old_size);
    }

    return new_ptr;
}

inline void
free_ptr(void *p)
{
    void *ptr = *(void**) p;
    if (ptr == NULL) {
        return;
    }

    free(ptr);
}

inline void
fclose_ptr(FILE **p)
{
    if (*p == NULL || *p == stdin || *p == stdout || *p == stderr) {
        return;
    }

    fclose(*p);
}
