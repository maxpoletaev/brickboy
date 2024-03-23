#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    char *ptr;
    size_t len;
    size_t cap;
} String;

String str_new(void);

String str_new_size(size_t size);

String str_new_from(const char *from);

String str_grow(String str, size_t newlen);

void str_free(String *str);

String str_clone(String str);

String str_add(String str, const char *add);

String str_addc(String str, char add);

bool str_cmp(String str, String other);

String str_addf(String str, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

String str_pad(String str, size_t len, char pad);

String str_trunc(String str, size_t len);
