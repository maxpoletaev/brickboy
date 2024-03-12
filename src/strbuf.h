#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct Strbuf {
    char *str;
    size_t cap;
    size_t pos; // position excluding null terminator < cap-1
} Strbuf;

Strbuf *strbuf_new(size_t size);

void strbuf_free(Strbuf **buf);

void strbuf_add(Strbuf *buf, const char *str);

void strbuf_addf(Strbuf *buf, const char *fmt, ...);

void strbuf_pad(Strbuf *buf, size_t len, char pad);

void strbuf_clear(Strbuf *buf);

