#ifndef BRICKBOY_STRBUF_H
#define BRICKBOY_STRBUF_H

#include <stdbool.h>

#define GB_STRBUF_MAX 256

typedef struct {
    char data[GB_STRBUF_MAX];
    char *ptr;
    size_t pos;
} gb_strbuf_t;

void gb_strbuf_init(gb_strbuf_t *buf);
void gb_strbuf_add(gb_strbuf_t *buf, const char *str);
void gb_strbuf_addf(gb_strbuf_t *buf, const char *fmt, ...);
void gb_strbuf_pad(gb_strbuf_t *buf, size_t len, char pad);
char *gb_strbuf_data(gb_strbuf_t *buf);

#endif //BRICKBOY_STRBUF_H
