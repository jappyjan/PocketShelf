#include "app/browser.h"
#include "app/state.h"
#include "core/book.h"
#include "core/text.h"
#include "library/client.h"
#include "platform/runtime.h"
#include "storage/paths.h"
#include "ui/list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
Book browser_book(App *app, int index) {
  Book b = {0};
  book_expand(&app->browser.books[index].summary, &b);
  return b;
}

static void *run_browse(void *context) {
  App *app = context;
  if (app->browser.job.type == COVER)
    app->browser.job.ok =
        library_cover(&app->browser.job.account, &app->browser.job.book,
                      COVER_DIR, &app->browser.job.transfer);
  else
    app->browser.job.ok =
        library_search(&app->browser.job.account, app->browser.job.query,
                       app->browser.job.format, app->browser.job.page,
                       app->browser.job.popular, &app->browser.job.results,
                       &app->browser.job.transfer, app->browser.job.error,
                       sizeof(app->browser.job.error));
  pthread_mutex_lock(&app->lock);
  app->browser.job.done = 1;
  pthread_mutex_unlock(&app->lock);
  return NULL;
}

void browser_cancel(App *app) {
  if (app->browser.active) {
    app->browser.discard = 1;
    transfer_cancel(&app->browser.job.transfer);
  }
  app->browser.more = 0;
  app->gesture.down = 0;
}

void browser_prefetch(App *app) {
  if (app->screen != LIST || app->foreground.active || app->keyboard_active ||
      !signed_in(app) || app->browser.active)
    return;
  int needs_page = app->browser.more && !app->browser.failed &&
                   (app->browser.page < 3 ||
                    app->browser.count * ROW_HEIGHT - app->browser.offset <=
                        3 * (LIST_BOTTOM - LIST_TOP));
  int cover_index = -1;
  if (!needs_page) {
    int end =
        (app->browser.offset + 3 * (LIST_BOTTOM - LIST_TOP)) / ROW_HEIGHT + 1;
    if (end > app->browser.count)
      end = app->browser.count;
    for (int i = app->browser.offset / ROW_HEIGHT; i < end; i++)
      if (!app->browser.books[i].cover_attempted &&
          app->browser.books[i].summary.cover_url[0]) {
        cover_index = i;
        break;
      }
    if (cover_index < 0)
      return;
  }
  memset(&app->browser.job, 0, sizeof(app->browser.job));
  app->browser.job.type = needs_page ? SEARCH : COVER;
  app->browser.job.account = app->account;
  app->browser.job.page = needs_page ? app->browser.page + 1 : cover_index;
  app->browser.job.popular = app->browser.popular;
  if (!needs_page)
    app->browser.job.book = browser_book(app, cover_index);
  snprintf(app->browser.job.query, sizeof(app->browser.job.query), "%s",
           app->query);
  snprintf(app->browser.job.format, sizeof(app->browser.job.format), "%s",
           app->format);
  app->browser.discard = 0;
  if (pthread_create(&app->browser.thread, NULL, run_browse, app) != 0) {
    app->browser.failed = 1;
    return;
  }
  app->browser.active = 1;
  runtime_schedule(app, TIMER_BROWSE, 150);
}

int browser_append(App *app, const Results *incoming) {
  int needed = app->browser.count + incoming->count;
  if (needed > app->browser.capacity) {
    int capacity = app->browser.capacity ? app->browser.capacity * 2 : 32;
    if (capacity < needed)
      capacity = needed;
    BrowseBook *grown = realloc(app->browser.books,
                                (size_t)capacity * sizeof(*app->browser.books));
    if (!grown)
      return 0;
    app->browser.books = grown;
    app->browser.capacity = capacity;
  }
  for (int i = 0; i < incoming->count; i++) {
    const BookSummary *b = &incoming->books[i];
    int duplicate = 0;
    for (int j = 0; j < app->browser.count; j++) {
      if (b->id[0] && !strcmp(b->id, app->browser.books[j].summary.id)) {
        duplicate = 1;
        break;
      }
    }
    if (!duplicate)
      app->browser.books[app->browser.count++] = (BrowseBook){.summary = *b};
  }
  return 1;
}

void browser_poll(App *app) {
  pthread_mutex_lock(&app->lock);
  int done = app->browser.job.done;
  pthread_mutex_unlock(&app->lock);
  if (!done) {
    runtime_schedule(app, TIMER_BROWSE, 150);
    return;
  }
  pthread_join(app->browser.thread, NULL);
  app->browser.active = 0;
  if (app->browser.discard) {
    app->browser.discard = 0;
    browser_prefetch(app);
    return;
  }
  if (app->browser.job.type == COVER) {
    int index = app->browser.job.page;
    app->browser.books[index].cover_attempted = 1;
    snprintf(app->browser.books[index].summary.cover_path, PATH_CAP, "%s",
             app->browser.job.book.cover_path);
    if (app->browser.job.ok && app->screen == LIST && !app->foreground.active &&
        !app->keyboard_active &&
        index * ROW_HEIGHT < app->browser.offset + LIST_BOTTOM - LIST_TOP &&
        (index + 1) * ROW_HEIGHT > app->browser.offset) {
      app->scroll_update = 1;
      app_render(app);
      app->scroll_update = 0;
    }
    browser_prefetch(app);
    return;
  }
  int previous_count = app->browser.count;
  int previous_more = app->browser.more;
  if (!app->browser.job.ok || !browser_append(app, &app->browser.job.results))
    app->browser.failed = 1;
  else {
    app->browser.page = app->browser.job.page;
    app->browser.more = app->browser.job.results.has_more &&
                        app->browser.count > previous_count;
  }
  browser_prefetch(app);
  /* Appending below the viewport needs no display refresh at all. */
  if (app->screen == LIST && !app->foreground.active && !app->keyboard_active &&
      (previous_count * ROW_HEIGHT <
           app->browser.offset + LIST_BOTTOM - LIST_TOP ||
       app->browser.failed || previous_more != app->browser.more)) {
    app->scroll_update = 1;
    app_render(app);
    app->scroll_update = 0;
  }
}

void browser_begin(App *app, int browse) {
  browser_cancel(app);
  app->browser.count = app->browser.offset = app->browser.page = 0;
  app->browser.popular = browse;
  app->browser.more = 1;
  app->browser.failed = 0;
  app->notice[0] = 0;
  app->screen = LIST;
  /* Connection UI is allowed only when entering a new search. */
  NetConnect2(NULL, 1);
  browser_prefetch(app);
  app_render(app);
}

void browser_shutdown(App *app) {
  browser_cancel(app);
  if (app->browser.active)
    pthread_join(app->browser.thread, NULL);
  runtime_clear(TIMER_BROWSE);
  secure_wipe(&app->browser.job, sizeof(app->browser.job));
  free(app->browser.books);
  app->browser.books = NULL;
  app->browser.active = app->browser.count = app->browser.capacity = 0;
}
