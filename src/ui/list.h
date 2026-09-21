#ifndef POCKETSHELF_SRC_UI_LIST_H
#define POCKETSHELF_SRC_UI_LIST_H
#include "ui/widgets.h"
#define LIST_TOP 154
#define LIST_BOTTOM 784
#define ROW_HEIGHT 120
typedef void (*ListRowRenderer)(void *, int index, int y);
typedef struct {
  int count, row_height, action_base;
  int *offset;
  ListRowRenderer draw_row;
  void *context;
} ScrollList;
void scroll_list_draw(Ui *, const ScrollList *);
int scroll_list_clamp(const ScrollList *, int offset);
int scroll_list_step(const ScrollList *);
#endif
