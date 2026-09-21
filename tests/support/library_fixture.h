#ifndef POCKETSHELF_TESTS_SUPPORT_LIBRARY_FIXTURE_H
#define POCKETSHELF_TESTS_SUPPORT_LIBRARY_FIXTURE_H
#include "library/client.h"
typedef struct {
  int success, search_fail, search_end, search_calls, hold_search, cover_calls;
} LibraryFixture;
extern LibraryFixture fixture;

#endif
