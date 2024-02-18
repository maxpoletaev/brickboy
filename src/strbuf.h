#ifndef BRICKBOY_STRBUF_H
#define BRICKBOY_STRBUF_H

#include <stdbool.h>

typedef struct {
    char *data;
    size_t cap;
    size_t pos; // position excluding null terminator < cap-1
} Strbuf;

void strbuf_init(Strbuf *buf, char *data, size_t size);

void strbuf_add(Strbuf *buf, const char *str);

void strbuf_addf(Strbuf *buf, const char *fmt, ...);

void strbuf_pad(Strbuf *buf, size_t len, char pad);

#endif //BRICKBOY_STRBUF_H
