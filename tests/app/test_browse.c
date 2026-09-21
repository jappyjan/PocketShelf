#include "support/app_test.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
void test_browse(App *app) {
  strcpy(app->query, "First");
  browser_begin(app, 0);
  int initial_updates = mock_ui.full_updates;
  test_drain_browse(app);
  assert(app->browser.page >= 3 && app->browser.count > BOOKS_PER_PAGE &&
         !app->foreground.active);
  assert(mock_ui.full_updates == initial_updates &&
         mock_ui.partial_updates > 0);
  assert(app->browser.offset == 0 && !strstr(mock_ui.drawn, "Next") &&
         !strstr(mock_ui.drawn, "Previous"));
  for (int i = 0; i < app->ui.button_count; i++)
    assert(app->ui.buttons[i].action != 30 && app->ui.buttons[i].action != 31);
  app_event(app, EVT_KEYPRESS, IV_KEY_NEXT2, 0);
  assert(app->browser.offset > 0 &&
         "Secondary hardware page key must scroll the list");
  test_drain_browse(app);
  app_scroll(app, 0);
  app_event(app, EVT_POINTERDOWN, sx(250), sy(400));
  app_event(app, EVT_POINTERDRAG, sx(250), sy(200));
  assert(app->browser.offset > 0 &&
         "Native drag event must move the list before release");
  app_event(app, EVT_POINTERUP, sx(250), sy(200));
  test_drain_browse(app);
  app_scroll(app, 0);
  browser_begin(app, 0);
  test_drain_browse(app);
  int before_count = app->browser.count;
  int before_partial = mock_ui.partial_updates;
  int before_calls = fixture.search_calls;
  app_scroll(app, 400);
  test_drain_browse(app);
  assert(app->browser.offset == 400 && app->browser.count > before_count);
  assert(mock_ui.partial_updates == before_partial + 1 &&
         "Offscreen appends must not repaint");
  assert(fixture.search_calls > before_calls);
  int position = app->browser.offset;
  app->screen = DETAIL;
  app_action(app, ACTION_BACK);
  assert(app->screen == LIST && app->browser.offset == position);
  app_event(app, EVT_POINTERDOWN, sx(250), sy(400));
  app_event(app, EVT_POINTERMOVE, sx(250), sy(200));
  app_event(app, EVT_POINTERUP, sx(250), sy(200));
  assert(app->screen == LIST && app->browser.offset >= position + 199 &&
         !app->foreground.active);
  test_drain_browse(app);
  app_scroll(app, -100);
  assert(app->browser.offset == 0);
  fixture.search_fail = 1;
  app_scroll(app, app->browser.count * ROW_HEIGHT);
  position = app->browser.offset;
  before_count = app->browser.count;
  test_drain_browse(app);
  assert(app->browser.failed && app->browser.count == before_count &&
         app->browser.offset == position);
  before_calls = fixture.search_calls;
  browser_prefetch(app);
  assert(fixture.search_calls == before_calls && !app->browser.active);
  fixture.search_fail = 0;
  app_action(app, ACTION_RETRY);
  test_drain_browse(app);
  assert(!app->browser.failed && app->browser.count > before_count &&
         app->browser.offset == position);
  fixture.search_end = 2;
  browser_begin(app, 0);
  test_drain_browse(app);
  assert(app->browser.page == 2 && app->browser.count == 8 &&
         !app->browser.more);
  before_count = app->browser.count;
  assert(browser_append(app, &app->browser.job.results) &&
         app->browser.count == before_count);
  __sync_lock_test_and_set(&fixture.hold_search, 1);
  browser_begin(app, 0);
  assert(app->browser.active);
  strcpy(app->query, "Replacement");
  browser_begin(app, 0);
  __sync_lock_test_and_set(&fixture.hold_search, 0);
  test_drain_browse(app);
  Book first = browser_book(app, 0);
  assert(app->browser.count == 8 && app->browser.offset == 0 &&
         strstr(first.title, "Replacement"));
  /* A completion on another screen must never navigate back to the list. */
  browser_begin(app, 0);
  app->screen = HOME;
  test_drain_browse(app);
  assert(app->screen == HOME);
  /* Real Popular response shape: 100 rows in one collection, no pagination. */
  browser_begin(app, 1);
  test_drain_browse(app);
  assert(app->browser.count == 100 && !app->browser.more &&
         app->browser.page == 1);
  assert(fixture.cover_calls > 0 && fixture.cover_calls < 100 &&
         "Covers load only near the viewport");
  int last_row_bottom = 0;
  for (int i = 0; i < app->ui.button_count; i++)
    if (app->ui.buttons[i].action >= BOOK_ACTION)
      last_row_bottom = app->ui.buttons[i].y + app->ui.buttons[i].h;
  assert(last_row_bottom == LIST_BOTTOM &&
         "Books must fill the lower viewport");
  app_event(app, EVT_KEYPRESS, IV_KEY_NEXT, 0);
  assert(app->browser.offset > 0);
  test_drain_browse(app);
  app_event(app, EVT_KEYPRESS, IV_KEY_PREV2, 0);
  assert(app->browser.offset == 0);
  app_event(app, EVT_POINTERDOWN, sx(200), sy(600));
  app_event(app, EVT_POINTERDRAG, sx(200), sy(300));
  app_event(app, EVT_POINTERUP, sx(200), sy(300));
  assert(app->browser.offset >= 299 && app->screen == LIST);
  test_drain_browse(app);
  app->screen = LOCAL;
  app->downloads.count = 20;
  app_render(app);
  app_event(app, EVT_KEYPRESS, IV_KEY_NEXT, 0);
  assert(app->local_scroll > 0 && app->screen == LOCAL);
  assert(active_list(app).offset == &app->local_scroll);
  app_event(app, EVT_KEYPRESS, IV_KEY_PREV2, 0);
  assert(app->local_scroll == 0);
  app_event(app, EVT_POINTERDOWN, sx(200), sy(600));
  app_event(app, EVT_POINTERDRAG, sx(200), sy(300));
  app_event(app, EVT_POINTERUP, sx(200), sy(300));
  assert(app->local_scroll >= 299 && app->screen == LOCAL);
  browser_cancel(app);
  free(app->browser.books);
  app->browser.books = NULL;
  puts("Infinite scrolling, prefetch, retry and stale-search regressions "
       "passed.");
  puts("Sign-in keyboard, login-only navigation and color regression passed.");
}
