#include "ini.h"

#include <stdlib.h>
#include <string.h>

struct ini {
    ini_entry *entries;
    size_t count;
    size_t cap;
    char section[INI_MAX_SECTION];
};

static int ini_reserve(ini_t *ini)
{
    if (ini->count < ini->cap)
        return 1;

    size_t cap = ini->cap ? ini->cap * 2 : 16;
    ini_entry *grown = realloc(ini->entries, cap * sizeof *grown);
    if (grown == NULL)
        return 0;

    ini->entries = grown;
    ini->cap = cap;
    return 1;
}

static void copy_trunc(char *dst, size_t dst_size, const char *src, size_t len)
{
    if (len >= dst_size)
        len = dst_size - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
}

/* Pointer to the first byte after trailing spaces, CR and tabs. */
static const char *trim_end(const char *start, const char *end)
{
    while (end > start && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r'))
        end--;
    return end;
}

ini_t *ini_parse(const char *buf, size_t len)
{
    ini_t *ini = calloc(1, sizeof *ini);
    if (ini == NULL)
        return NULL;

    size_t pos = 0;
    while (pos < len) {
        size_t line_start = pos;
        while (pos < len && buf[pos] != '\n')
            pos++;

        const char *line = buf + line_start;
        const char *end = trim_end(line, buf + pos);
        if (pos < len)
            pos++; /* consume the newline */

        while (line < end && (*line == ' ' || *line == '\t'))
            line++;

        if (line == end || *line == ';' || *line == '#')
            continue;

        if (*line == '[') {
            const char *close = memchr(line, ']', (size_t)(end - line));
            if (close == NULL)
                continue;
            copy_trunc(ini->section, sizeof ini->section, line + 1,
                       (size_t)(close - line - 1));
            continue;
        }

        const char *eq = memchr(line, '=', (size_t)(end - line));
        if (eq == NULL)
            continue;

        const char *key_end = trim_end(line, eq);
        if (key_end == line || !ini_reserve(ini)) {
            if (key_end == line)
                continue;
            ini_free(ini);
            return NULL;
        }

        const char *value = eq + 1;
        while (value < end && (*value == ' ' || *value == '\t'))
            value++;

        ini_entry *entry = &ini->entries[ini->count++];
        size_t section_len = strlen(ini->section);
        copy_trunc(entry->section, sizeof entry->section, ini->section, section_len);
        copy_trunc(entry->key, sizeof entry->key, line, (size_t)(key_end - line));
        copy_trunc(entry->value, sizeof entry->value, value, (size_t)(end - value));
    }

    return ini;
}

size_t ini_count(const ini_t *ini)
{
    return ini->count;
}

const ini_entry *ini_at(const ini_t *ini, size_t idx)
{
    return idx < ini->count ? &ini->entries[idx] : NULL;
}

const char *ini_get(const ini_t *ini, const char *section, const char *key)
{
    const char *value = NULL;

    for (size_t i = 0; i < ini->count; i++) {
        const ini_entry *entry = &ini->entries[i];
        if (strcmp(entry->section, section) == 0 && strcmp(entry->key, key) == 0)
            value = entry->value;
    }

    return value;
}

void ini_free(ini_t *ini)
{
    if (ini == NULL)
        return;
    free(ini->entries);
    free(ini);
}
