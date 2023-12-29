#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "common.h"
#include "strbuf.h"

void
gb_strbuf_init(gb_strbuf_t *buf)
{
    memset(buf, 0, sizeof(*buf));
    buf->ptr = buf->data;
    buf->pos = 0;
}

void
gb_strbuf_pad(gb_strbuf_t *buf, size_t size, char pad)
{
    if (buf->pos < size) {
        memset(buf->ptr, pad, size - buf->pos);
        buf->ptr = buf->data + size;
        buf->pos = size;
    }
}

void
gb_strbuf_add(gb_strbuf_t *buf, const char *str)
{
    size_t len = strlen(str) + 1;
    if (buf->pos + len >= sizeof(buf->data))
        GB_PANIC("buffer overflow");

    memcpy(buf->ptr, str, len);
    buf->ptr += len;
    buf->pos += len;
}

void
gb_strbuf_addf(gb_strbuf_t *buf, const char *fmt, ...)
{
    char *head = buf->ptr;
    size_t size = sizeof(buf->data) - buf->pos;

    va_list args;
    va_start(args, fmt);
    int written = vsnprintf(head, size, fmt, args);
    va_end(args);

    if (written < 0)
        GB_PANIC("vsnprintf failed");

    buf->pos += (size_t) written;
    if (buf->pos >= sizeof(buf->data))
        GB_PANIC("buffer overflow");

    buf->ptr = buf->data + buf->pos;
}

char *
gb_strbuf_data(gb_strbuf_t *buf)
{
    return buf->data;
}

#ifdef GB_STRBUF_MAIN
int
main(int argc, char **argv)
{
    gb_strbuf_t buf;
    gb_strbuf_init(&buf);

    gb_strbuf_addf(&buf, "col:%d", 1);
    gb_strbuf_pad(&buf, 10, ' ');
    gb_strbuf_addf(&buf, "col:%d", 2);
    gb_strbuf_pad(&buf, 25, ' ');
    gb_strbuf_addf(&buf, "col:%d", 3);

    printf("%s\n", buf.data);

    return 0;
}
#endif //GB_STRBUF_MAIN
