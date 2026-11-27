#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ini.h"

/* strlen that stops at the field capacity, POSIX strnlen is not in C99. */
static size_t bounded_len(const char *s, size_t cap)
{
    size_t n = 0;
    while (n < cap && s[n] != '\0')
        n++;
    return n;
}

/* Harness contract for src/ini.c:
 *  - parsing arbitrary bytes must not crash or leak (ASan),
 *  - every stored entry must be printable without overrun,
 *  - a stored key must be findable through the public lookup.
 * A miss here means the parser rewrote a key it accepted. */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    ini_t *ini = ini_parse((const char *)data, size);
    if (ini == NULL)
        return 0;

    for (size_t i = 0; i < ini_count(ini); i++) {
        const ini_entry *entry = ini_at(ini, i);
        if (entry == NULL)
            abort();

        size_t section_len = bounded_len(entry->section, sizeof entry->section);
        size_t key_len = bounded_len(entry->key, sizeof entry->key);
        size_t value_len = bounded_len(entry->value, sizeof entry->value);

        if (section_len >= sizeof entry->section || key_len >= sizeof entry->key ||
            value_len >= sizeof entry->value)
            abort();

        const char *found = ini_get(ini, entry->section, entry->key);
        if (found == NULL)
            abort();
    }

    ini_free(ini);
    return 0;
}
