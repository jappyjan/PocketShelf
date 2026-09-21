#ifndef POCKETSHELF_SRC_PLATFORM_LIBRARY_H
#define POCKETSHELF_SRC_PLATFORM_LIBRARY_H
#include "app/app.h"
int register_book(const char *);
void repair_library(App *);
void start_library_repair(App *);
void stop_library_repair(App *);

#endif
