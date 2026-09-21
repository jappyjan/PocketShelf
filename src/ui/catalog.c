#include "ui/catalog.h"
#include "app/browser.h"
#include "app/state.h"
#include "core/book.h"
static void draw_book_row(void *context, int index, int y) {
  App *app = context;
  Book book = browser_book(app, index);
  DrawLine(sx(108), sy(y + 116), sx(576), sy(y + 116), 0xcccccc);
  draw_cover(&app->ui, &book, 26, y + 4, 66, 104);
  text_at(108, y + 2, 466, 54, book.title, app->ui.font);
  char meta[400];
  book_overview(&book, meta, sizeof(meta));
  text_at(108, y + 62, 466, 48, meta, app->ui.small);
}

static void draw_local_row(void *context, int index, int y) {
  App *app = context;
  text_at(28, y + 12, 544, 52, app->downloads.names[index], app->ui.font);
  DrawLine(sx(24), sy(y + 71), sx(576), sy(y + 71), 0xcccccc);
}

int is_list_screen(App *app) {
  return app->screen == LIST || app->screen == LOCAL;
}

ScrollList active_list(App *app) {
  if (app->screen == LOCAL)
    return (ScrollList){app->downloads.count, 72,
                        LOCAL_ACTION,         &app->local_scroll,
                        draw_local_row,       app};
  return (ScrollList){app->browser.count,   ROW_HEIGHT,    BOOK_ACTION,
                      &app->browser.offset, draw_book_row, app};
}

void draw_scroll_list(App *app, const char *title, const char *empty) {
  ScrollList list = active_list(app);
  int retry = app->screen == LIST && app->browser.failed;
  text_at(24, 103,
          app->screen == LOCAL ? 552
          : retry              ? 280
                               : 414,
          46, title, app->ui.font);
  if (app->screen == LIST) {
    if (retry)
      button(&app->ui, 320, 96, 124, 48, "Retry", ACTION_RETRY);
    button(&app->ui, 452, 96, 124, 48, "Search", ACTION_SEARCH);
  }
  if (!list.count)
    text_at(24, 210, 552, 100, empty, app->ui.font);
  scroll_list_draw(&app->ui, &list);
}
