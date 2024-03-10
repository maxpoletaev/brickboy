#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

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

const char * __attribute__((format(printf, 1, 2)))
strfmt(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    static char buf[1024];
    size_t size = vsnprintf(buf, sizeof(buf), fmt, args);

    if (size < 0) {
        PANIC("failed to format string");
    } else if (size >= sizeof(buf)) {
        PANIC("formatted string is too long");
    }

    va_end(args);

    return buf;
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
