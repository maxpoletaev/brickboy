#ifndef BRICKBOY_STRBUF_H
#define BRICKBOY_STRBUF_H

#include <stdbool.h>

typedef struct {
    char *data;
    size_t cap;
    size_t pos; // position excluding null terminator < cap-1
} StrBuf;

void strbuf_init(StrBuf *buf, char *data, size_t size);

void strbuf_add(StrBuf *buf, const char *str);

void strbuf_addf(StrBuf *buf, const char *fmt, ...);

void strbuf_pad(StrBuf *buf, size_t len, char pad);

#endif //BRICKBOY_STRBUF_H
