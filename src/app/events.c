#include "app/browser.h"
#include "app/jobs.h"
#include "app/state.h"
#include "core/text.h"
#include "platform/library.h"
#include "platform/runtime.h"
#include "storage/account.h"
#include "storage/paths.h"
#include "ui/catalog.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
void app_scroll(App *app, int offset) {
  if (app->foreground.active || app->keyboard_active || !is_list_screen(app))
    return;
  ScrollList list = active_list(app);
  int *position = list.offset;
  offset = scroll_list_clamp(&list, offset);
  if (*position != offset) {
    *position = offset;
    app->scroll_update = 1;
    app_render(app);
    app->scroll_update = 0;
  }
  browser_prefetch(app);
}

static void app_tap(App *app, int p1, int p2) {
  int x = p1 * 600 / ScreenWidth(), y = p2 * 800 / ScreenHeight();
  for (int i = 0; i < app->ui.button_count; i++) {
    Button b = app->ui.buttons[i];
    if (x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h) {
      app_action(app, b.action);
      break;
    }
  }
}

int app_event(App *app, int event, int p1, int p2) {
  if (event == EVT_INIT) {
    icanvas *canvas = GetCanvas();
    fprintf(stderr, "[pocketshelf] canvas_depth=%d\n",
            canvas ? canvas->depth : 0);
    fflush(stderr);
    mkdir("/mnt/ext1/Books", 0755);
    mkdir(BOOK_DIR, 0755);
    mkdir(DATA_DIR, 0700);
    mkdir(COVER_DIR, 0700);
    account_load(CONFIG_FILE, &app->account);
    snprintf(app->email, sizeof(app->email), "%s", app->account.email);
    app->screen = signed_in(app) ? HOME : SIGNIN;
    app->ui.font = OpenFont("DejaVuSans", sx(20), 1);
    app->ui.small = OpenFont("DejaVuSans", sx(16), 1);
    app->ui.large = OpenFont("DejaVuSans", sx(29), 1);
    if (access(INDEX_REPAIR_MARKER, F_OK) != 0)
      start_library_repair(app);
  } else if (event == EVT_SHOW) {
    if (!app->keyboard_active)
      app_render(app);
  } else if (event == EVT_POINTERDOWN && !app->foreground.active &&
             !app->keyboard_active && is_list_screen(app)) {
    int y = p2 * 800 / ScreenHeight();
    app->gesture.down = y >= LIST_TOP && y < LIST_BOTTOM;
    app->gesture.y = y;
    app->gesture.offset = *active_list(app).offset;
    app->gesture.dragged = 0;
  } else if ((event == EVT_POINTERMOVE || event == EVT_POINTERDRAG ||
              event == EVT_POINTERUP) &&
             app->gesture.down) {
    int delta = app->gesture.y - p2 * 800 / ScreenHeight();
    if (abs(delta) > 12)
      app->gesture.dragged = 1;
    if (app->gesture.dragged)
      app_scroll(app, app->gesture.offset + delta);
    if (event != EVT_POINTERUP)
      return 0;
    app->gesture.down = 0;
    if (app->gesture.dragged)
      return 0;
    /* A stationary touch is handled as a tap below. */
    app_tap(app, p1, p2);
  } else if (event == EVT_POINTERCANCEL) {
    app->gesture.down = app->gesture.dragged = 0;
  } else if (event == EVT_POINTERUP) {
    app_tap(app, p1, p2);
  } else if (event == EVT_KEYPRESS) {
    if (p1 == IV_KEY_BACK)
      app_action(app, ACTION_BACK);
    else if (!app->foreground.active && is_list_screen(app) &&
             (p1 == IV_KEY_NEXT || p1 == IV_KEY_NEXT2 || p1 == IV_KEY_PREV ||
              p1 == IV_KEY_PREV2)) {
      ScrollList list = active_list(app);
      int step = scroll_list_step(&list);
      int forward = p1 == IV_KEY_NEXT || p1 == IV_KEY_NEXT2;
      app_scroll(app, *list.offset + (forward ? step : -step));
    } else if (!app->foreground.active && app->screen == DETAIL &&
               (p1 == IV_KEY_NEXT || p1 == IV_KEY_NEXT2))
      app_action(app, ACTION_TEXT_NEXT);
    else if (!app->foreground.active && app->screen == DETAIL &&
             (p1 == IV_KEY_PREV || p1 == IV_KEY_PREV2))
      app_action(app, ACTION_TEXT_PREVIOUS);
  } else if (event == EVT_EXIT) {
    browser_shutdown(app);
    jobs_shutdown(app);
    stop_library_repair(app);
    runtime_clear(TIMER_KEYBOARD);
    runtime_clear(TIMER_SEARCH);
    secure_wipe(app->password, sizeof(app->password));
    secure_wipe(app->edit, sizeof(app->edit));
    CloseFont(app->ui.font);
    CloseFont(app->ui.small);
    CloseFont(app->ui.large);
    secure_wipe(&app->account, sizeof(app->account));
    pthread_mutex_destroy(&app->lock);
  }
  return 0;
}
