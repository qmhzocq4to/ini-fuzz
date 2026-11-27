#ifndef INI_H
#define INI_H

#include <stddef.h>

/* Fixed capacities: the parser truncates instead of allocating per field.
 * Every truncated write is a potential key collision, see README threat model. */
#define INI_MAX_SECTION 64
#define INI_MAX_KEY 64
#define INI_MAX_VALUE 256

typedef struct {
    char section[INI_MAX_SECTION];
    char key[INI_MAX_KEY];
    char value[INI_MAX_VALUE];
} ini_entry;

typedef struct ini ini_t;

/* Parses `len` bytes of `buf`. Returns NULL only on allocation failure.
 * Malformed lines are skipped, never fatal: config files are untrusted input. */
ini_t *ini_parse(const char *buf, size_t len);

size_t ini_count(const ini_t *ini);
const ini_entry *ini_at(const ini_t *ini, size_t idx);

/* Last value wins, matching the lookup order services actually use. */
const char *ini_get(const ini_t *ini, const char *section, const char *key);

void ini_free(ini_t *ini);

#endif /* INI_H */
