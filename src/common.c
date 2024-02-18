#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

inline char *
to_cstring(const char *data, size_t size)
{
    char *str = NULL;

    if (data[size] == '\0') {
        str = must_alloc(size);
        strncpy(str, data, size);
    } else {
        str = must_alloc(size + 1);
        strncpy(str, data, size);
        str[size] = '\0';
    }

    return str;
}

inline void *
must_alloc(size_t size)
{
    void *ptr = calloc(1, size);

    if (ptr == NULL) {
        PANIC("failed to allocate %zu bytes", size);
    }

    return ptr;
}
