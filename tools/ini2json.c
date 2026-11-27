/* ini2json: parse one config file and print the stored entries as JSON.
 * Built without libFuzzer so a crash report can be reproduced and diffed
 * between two parser builds. Exits 2 if two entries share a (section, key)
 * pair, i.e. one config value silently shadows another. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ini.h"

static void json_escape(const char *s)
{
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        switch (c) {
        case '"':  fputs("\\\"", stdout); break;
        case '\\': fputs("\\\\", stdout); break;
        case '\t': fputs("\\t", stdout); break;
        case '\r': fputs("\\r", stdout); break;
        case '\n': fputs("\\n", stdout); break;
        default:
            if (c < 0x20)
                printf("\\u%04x", c);
            else
                putchar(c);
        }
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: ini2json FILE\n");
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (f == NULL) {
        perror(argv[1]);
        return 1;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        fprintf(stderr, "%s: not seekable\n", argv[1]);
        return 1;
    }
    long len = ftell(f);
    rewind(f);

    char *buf = malloc((size_t)len + 1);
    if (buf == NULL || fread(buf, 1, (size_t)len, f) != (size_t)len) {
        fprintf(stderr, "%s: short read\n", argv[1]);
        fclose(f);
        free(buf);
        return 1;
    }
    fclose(f);

    ini_t *ini = ini_parse(buf, (size_t)len);
    free(buf);
    if (ini == NULL) {
        fputs("allocation failed\n", stderr);
        return 1;
    }

    int shadowed = 0;
    fputs("[\n", stdout);
    for (size_t i = 0; i < ini_count(ini); i++) {
        const ini_entry *e = ini_at(ini, i);
        if (ini_get(ini, e->section, e->key) == NULL) {
            fprintf(stderr, "unreachable entry %zu: [%s] %s\n", i, e->section, e->key);
            shadowed++;
        }
        for (size_t j = i + 1; j < ini_count(ini); j++) {
            const ini_entry *other = ini_at(ini, j);
            if (strcmp(other->section, e->section) == 0 &&
                strcmp(other->key, e->key) == 0) {
                fprintf(stderr, "shadowed entry %zu by %zu: [%s] %s\n", i, j, e->section, e->key);
                shadowed++;
                break;
            }
        }
        printf("  {\"section\": \"");
        json_escape(e->section);
        fputs("\", \"key\": \"", stdout);
        json_escape(e->key);
        fputs("\", \"value\": \"", stdout);
        json_escape(e->value);
        printf("\"}%s\n", i + 1 < ini_count(ini) ? "," : "");
    }
    fputs("]\n", stdout);
    ini_free(ini);

    return shadowed ? 2 : 0;
}
