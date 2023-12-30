#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "shared.h"
#include "strbuf.h"

static inline void
gb_check_overflow(gb_strbuf_t *buf, size_t newpos)
{
    if (newpos+1 >= sizeof(buf->data)) // +1 for null terminator
        GB_PANIC("buffer is too short: %zu>%zu", newpos+1, sizeof(buf->data));
}

void
gb_strbuf_init(gb_strbuf_t *buf)
{
    memset(buf, 0, sizeof(*buf));
    buf->pos = 0;
}

void
gb_strbuf_pad(gb_strbuf_t *buf, size_t newpos, char pad)
{
    if (buf->pos < newpos) {
        gb_check_overflow(buf, newpos);
        memset(buf->data + buf->pos, pad, newpos - buf->pos);
        buf->data[newpos+1] = '\0';
        buf->pos = newpos;
    }
}

void
gb_strbuf_add(gb_strbuf_t *buf, const char *str)
{
    size_t len = strlen(str);
    size_t newpos = buf->pos + len;

    gb_check_overflow(buf, newpos);
    memcpy(buf->data + buf->pos, str, len + 1);

    buf->pos = newpos;
}

void
gb_strbuf_addf(gb_strbuf_t *buf, const char *fmt, ...)
{
    int len = 0;
    char *ptr = buf->data + buf->pos;
    size_t left = sizeof(buf->data) - buf->pos;

    va_list args;
    va_start(args, fmt);
    len = vsnprintf(ptr, left, fmt, args);
    va_end(args);

    if (len < 0)
        GB_PANIC("vsnprintf failed");

    size_t newpos = buf->pos + (size_t) len;
    gb_check_overflow(buf, newpos);

    buf->pos = newpos;
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
