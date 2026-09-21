#ifndef POCKETSHELF_SRC_LIBRARY_JSON_H
#define POCKETSHELF_SRC_LIBRARY_JSON_H
/* Internal protocol helpers; callers own returned JSON trees. */
#include "cJSON.h"
#include <stddef.h>
const cJSON *json_field(const cJSON *, const char *);
const char *json_string(const cJSON *, const char *);
void json_identifier(const cJSON *, const char *, char *, size_t);
cJSON *json_decode(const char *, char *, size_t);

#endif
