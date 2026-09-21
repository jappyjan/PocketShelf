#ifndef POCKETSHELF_SRC_APP_BROWSER_H
#define POCKETSHELF_SRC_APP_BROWSER_H
#include "app/app.h"
#include "core/models.h"
void browser_begin(App *, int popular);
void browser_prefetch(App *);
void browser_poll(App *);
void browser_cancel(App *);
void browser_shutdown(App *);
int browser_append(App *, const Results *);
Book browser_book(App *, int index);

#endif
