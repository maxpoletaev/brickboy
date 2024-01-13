#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "shared.h"
#include "strbuf.h"

static inline void
strbuf_check(StrBuf *buf, size_t newpos)
{
    if (newpos+1 >= buf->cap) // +1 for null terminator
        PANIC("buffer is too short: %zu>%zu", newpos + 1, buf->cap);
}

void
strbuf_init(StrBuf *buf, char *data, size_t size)
{
    buf->data = data;
    buf->cap = size;
    buf->pos = 0;
}

void
strbuf_pad(StrBuf *buf, size_t len, char pad)
{
    if (buf->pos < len) {
        strbuf_check(buf, len);
        memset(buf->data + buf->pos, pad, len - buf->pos);
        buf->data[len + 1] = '\0';
        buf->pos = len;
    }
}

void
strbuf_add(StrBuf *buf, const char *str)
{
    size_t len = strlen(str);
    size_t newpos = buf->pos + len;

    strbuf_check(buf, newpos);
    memcpy(buf->data + buf->pos, str, len + 1);

    buf->pos = newpos;
}

void
strbuf_addf(StrBuf *buf, const char *fmt, ...)
{
    int len = 0;
    char *ptr = buf->data + buf->pos;
    size_t left = buf->cap - buf->pos;

    va_list args;
    va_start(args, fmt);
    len = vsnprintf(ptr, left, fmt, args);
    va_end(args);

    if (len < 0)
        PANIC("vsnprintf failed");

    size_t newpos = buf->pos + (size_t) len;
    strbuf_check(buf, newpos);

    buf->pos = newpos;
}
