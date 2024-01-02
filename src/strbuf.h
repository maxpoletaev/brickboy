#ifndef BRICKBOY_STRBUF_H
#define BRICKBOY_STRBUF_H

#include <stdbool.h>

typedef struct {
    char *data;
    size_t cap;
    size_t pos; // position excluding null terminator < cap-1
} gb_strbuf_t;

void gb_strbuf_init(gb_strbuf_t *buf, char *data, size_t size);

void gb_strbuf_add(gb_strbuf_t *buf, const char *str);

void gb_strbuf_addf(gb_strbuf_t *buf, const char *fmt, ...);

void gb_strbuf_pad(gb_strbuf_t *buf, size_t len, char pad);

#endif //BRICKBOY_STRBUF_H
