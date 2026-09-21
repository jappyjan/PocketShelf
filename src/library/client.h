#ifndef POCKETSHELF_SRC_LIBRARY_CLIENT_H
#define POCKETSHELF_SRC_LIBRARY_CLIENT_H
#include "core/models.h"
#include "net/transfer.h"
int library_login(Account *, const char *, const char *, Transfer *, char *,
                  size_t);
int library_search(const Account *, const char *, const char *, int, int,
                   Results *, Transfer *, char *, size_t);
int library_details(const Account *, Book *, Transfer *, char *, size_t);
int library_cover(const Account *, Book *, const char *, Transfer *);
int library_download(const Account *, const Book *, const char *, char *,
                     size_t, Transfer *, char *, size_t);

#endif
