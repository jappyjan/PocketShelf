#ifndef POCKETSHELF_UI_CATALOG_H
#define POCKETSHELF_UI_CATALOG_H
#include "app/app.h"
#include "ui/list.h"
int is_list_screen(App *);
ScrollList active_list(App *);
void draw_scroll_list(App *, const char *, const char *);
#endif
