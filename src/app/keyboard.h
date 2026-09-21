#ifndef POCKETSHELF_SRC_APP_KEYBOARD_H
#define POCKETSHELF_SRC_APP_KEYBOARD_H
#include "app/app.h"
void keyboard_finish(App *);
void keyboard_search_ready(App *);
void keyboard_complete(App *, char *value);
void keyboard_edit(App *, int kind, const char *, const char *, int flags);

#endif
