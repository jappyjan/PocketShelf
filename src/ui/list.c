#include "ui/list.h"
void scroll_list_draw(Ui *ui, const ScrollList *list) {
  SetClip(sx(24), sy(LIST_TOP), sx(552), sy(LIST_BOTTOM) - sy(LIST_TOP));
  for (int i = *list->offset / list->row_height; i < list->count; i++) {
    int y = LIST_TOP + i * list->row_height - *list->offset;
    if (y >= LIST_BOTTOM)
      break;
    list->draw_row(list->context, i, y);
    int top = y < LIST_TOP ? LIST_TOP : y;
    int bottom = y + list->row_height;
    if (bottom > LIST_BOTTOM)
      bottom = LIST_BOTTOM;
    ui->buttons[ui->button_count++] =
        (Button){24, top, 552, bottom - top, list->action_base + i};
  }
  SetClip(0, 0, ScreenWidth(), ScreenHeight());
}
int scroll_list_clamp(const ScrollList *list, int offset) {
  int maximum = list->count * list->row_height - (LIST_BOTTOM - LIST_TOP);
  if (maximum < 0)
    maximum = 0;
  if (offset < 0)
    offset = 0;
  if (offset > maximum)
    offset = maximum;
  return offset;
}
int scroll_list_step(const ScrollList *list) {
  return ((LIST_BOTTOM - LIST_TOP) / list->row_height - 1) * list->row_height;
}
