#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "shared.h"

inline char *
to_cstring(char *data, size_t size)
{
    char *str = malloc(size + 1);
    strncpy(str, data, size);
    str[size] = '\0';
    return str;
}
