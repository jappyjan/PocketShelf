#ifndef POCKETSHELF_SRC_LIBRARY_INTERNAL_H
#define POCKETSHELF_SRC_LIBRARY_INTERNAL_H
#include "library/client.h"
#include "library/json.h"
cJSON *library_api(const Account *, const char *, const char *, Transfer *,
                   char *, size_t);

#endif
