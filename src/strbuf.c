#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "common.h"
#include "strbuf.h"

static inline void
strbuf_check(Strbuf *buf, size_t newpos)
{
    if (newpos+1 >= buf->cap) { // +1 for null terminator
        PANIC("buffer is too short: %zu>%zu", newpos + 1, buf->cap);
    }
}

Strbuf
strbuf_new(size_t size)
{
    return (Strbuf) {
        .str = xalloc(size),
        .cap = size,
        .pos = 0,
    };
}

void
strbuf_free(Strbuf *buf)
{
    xfree(buf->str);
    buf->cap = 0;
    buf->pos = 0;
}

void
strbuf_pad(Strbuf *buf, size_t len, char pad)
{
    if (buf->pos < len) {
        strbuf_check(buf, len);
        memset(buf->str + buf->pos, pad, len - buf->pos);
        buf->str[len + 1] = '\0';
        buf->pos = len;
    }
}

void
strbuf_add(Strbuf *buf, const char *str)
{
    size_t len = strlen(str);
    size_t newpos = buf->pos + len;

    strbuf_check(buf, newpos);
    memcpy(buf->str + buf->pos, str, len + 1);

    buf->pos = newpos;
}

void __attribute__((format(printf, 2, 3)))
strbuf_addf(Strbuf *buf, const char *fmt, ...)
{
    int len = 0;
    char *ptr = buf->str + buf->pos;
    size_t left = buf->cap - buf->pos;

    va_list args;
    va_start(args, fmt);
    len = vsnprintf(ptr, left, fmt, args);
    va_end(args);

    if (len < 0) {
        PANIC("vsnprintf failed");
    }

    size_t newpos = buf->pos + (size_t) len;
    strbuf_check(buf, newpos);

    buf->pos = newpos;
}

void
strbuf_clear(Strbuf *buf)
{
    buf->pos = 0;
    buf->str[0] = '\0';
}
