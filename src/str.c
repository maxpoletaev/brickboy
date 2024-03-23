#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "common.h"
#include "str.h"

String
str_grow(String str, size_t newlen)
{
    size_t newcap = newlen + 1;
    if (newcap <= str.cap) {
        return str;
    }

    str.ptr = xrealloc(str.ptr, str.cap, newcap);
    str.cap = newcap;
    return str;
}

String
str_new(void)
{
    return str_new_size(0);
}

String
str_new_size(size_t size)
{
    String str = {
        .ptr = xalloc(size + 1),
        .cap = size + 1,
        .len = 0,
    };

    return str;
}

String
str_new_from(const char *from)
{
    size_t len = strlen(from);
    String str = str_new_size(len);
    memcpy(str.ptr, from, len+1);
    str.len = len;
    return str;
}

void
str_free(String *str)
{
    xfree(str->ptr);
    str->ptr = NULL;
    str->cap = 0;
    str->len = 0;
}

String
str_clone(String str)
{
    String newbuf = str_new_size(str.len);
    memcpy(newbuf.ptr, str.ptr, str.len+1);
    return newbuf;
}

String
str_pad(String str, size_t len, char pad)
{
    if (str.len < len) {
        str = str_grow(str, len);
        memset(str.ptr+str.len, pad, len-str.len);
        str.ptr[len] = '\0';
        str.len = len;
    }

    return str;
}

String
str_add(String str, const char *add)
{
    size_t len = strlen(add);
    size_t newlen = str.len + len;
    str = str_grow(str, newlen);

    memcpy(str.ptr+str.len, add, len+1);
    str.len = newlen;

    return str;
}

String
str_addc(String str, char add)
{
    size_t newlen = str.len + 1;
    str = str_grow(str, newlen);

    str.ptr[str.len] = add;
    str.ptr[str.len+1] = '\0';
    str.len = newlen;

    return str;
}

String
str_addf(String str, const char *fmt, ...)
{
    va_list args;

    // Determine the length of the formatted string.
    va_start(args, fmt);
    size_t wrlen = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (wrlen < 0) {
        PANIC("vsnprintf failed");
    }

    size_t newlen = str.len+wrlen;
    str = str_grow(str, newlen);

    // Write the formatted string
    va_start(args, fmt);
    wrlen = vsnprintf(str.ptr+str.len, str.cap-str.len, fmt, args);
    va_end(args);

    if (wrlen < 0) {
        PANIC("vsnprintf failed");
    }

    str.len = newlen;

    return str;
}

String
str_trunc(String str, size_t len)
{
    if (str.len > len) {
        str.ptr[len] = '\0';
        str.len = len;
    }

    return str;
}
