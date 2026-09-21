#include "app/state.h"
#include "core/book.h"
#include "ui/screens.h"
#include <stdio.h>
#include <string.h>
void screen_detail(App *app) {

  draw_cover(&app->ui, &app->selected, 24, 104, 104, 158);
  text_at(148, 104, 428, 94, app->selected.title, app->ui.large);
  text_at(148, 204, 428, 57, app->selected.author, app->ui.small);
  button(&app->ui, 24, 282, 266, 48,
         app->detail.tab ? "Description" : "[Description]", ACTION_DESCRIPTION);
  button(&app->ui, 310, 282, 266, 48,
         app->detail.tab ? "[Book info]" : "Book info", ACTION_INFO);
  detail_text(app);
  if (app->opened[0])
    button(&app->ui, 24, 650, 552, 56, "Open in PocketBook reader",
           ACTION_OPEN);
  else if (supported_format(app->selected.format)) {
    char label[80];
    snprintf(label, sizeof(label), "Download %s%s%s", app->selected.format,
             app->selected.size[0] ? " - " : "", app->selected.size);
    button(&app->ui, 24, 650, 552, 56, label, ACTION_DOWNLOAD);
  } else
    text_at(24, 650, 552, 56, "This format is not supported by PocketBook.",
            app->ui.small);
}

void detail_text(App *app) {
  const char *body =
      app->detail.tab ? app->selected.metadata : app->selected.description;
  if (!*body)
    body = app->detail.tab ? "No additional metadata supplied."
                           : "No description supplied for this edition.";
  size_t at = app->detail.offsets[app->detail.page], length = strlen(body);
  SetFont(app->ui.font, BLACK);
  for (int row = 0; row < 9 && at < length; row++) {
    char line[1024];
    size_t end = at, last_space = at, n = 0;
    while (end < length && body[end] != '\n' && n + 5 < sizeof(line)) {
      size_t next = end + 1;
      while (next < length && ((unsigned char)body[next] & 0xc0) == 0x80)
        next++;
      memcpy(line + n, body + end, next - end);
      n += next - end;
      line[n] = 0;
      if (StringWidth(line) > sx(548) && end > at)
        break;
      if (body[end] == ' ')
        last_space = end;
      end = next;
    }
    if (end < length && body[end] != '\n' && last_space > at)
      end = last_space;
    if (end == at && body[end] != '\n')
      end++;
    n = end - at;
    memcpy(line, body + at, n);
    line[n] = 0;
    text_at(26, 346 + row * 26, 548, 27, line, app->ui.font);
    at = end;
    if (body[at] == '\n' || body[at] == ' ')
      at++;
  }
  app->detail.more =
      at < length && app->detail.page + 1 < (int)(sizeof(app->detail.offsets) /
                                                  sizeof(*app->detail.offsets));
  if (app->detail.more)
    app->detail.offsets[app->detail.page + 1] = at;
  if (app->detail.page > 0)
    arrow_button(&app->ui, 24, 588, ACTION_TEXT_PREVIOUS, 0);
  char page_label[48];
  snprintf(page_label, sizeof(page_label), "Page %d", app->detail.page + 1);
  text_at(230, 600, 140, 30, page_label, app->ui.small);
  if (app->detail.more)
    arrow_button(&app->ui, 516, 588, ACTION_TEXT_NEXT, 1);
}
