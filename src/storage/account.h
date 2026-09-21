#ifndef POCKETSHELF_SRC_STORAGE_ACCOUNT_H
#define POCKETSHELF_SRC_STORAGE_ACCOUNT_H
#include "core/models.h"
int account_save(const char *path, const Account *);
void account_load(const char *path, Account *);
void account_forget(const char *path);

#endif
