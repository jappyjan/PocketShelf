#include "support/app_test.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
static void *completed_worker(void *unused) {
  (void)unused;
  return NULL;
}
void test_books(App *app) {
  strcpy(app->account.user, "123");
  strcpy(app->account.key, "fixture-session");
  app->screen = LIST;
  Results results = {0};
  results.count = 1;
  strcpy(results.books[0].title, "Missing metadata");
  strcpy(results.books[0].format, "mobi");
  assert(browser_append(app, &results));
  app_render(app);
  assert(!strstr(mock_ui.drawn, " | ") &&
         "Missing metadata must not create empty separators");
  char overview[400];
  Book missing = {0};
  book_overview(&missing, overview, sizeof(overview));
  assert(!*overview);
  strcpy(missing.author, "  Author  ");
  strcpy(missing.language, "  ");
  strcpy(missing.format, "epub");
  book_overview(&missing, overview, sizeof(overview));
  assert(!strcmp(overview, "Author | epub"));
  char handoff_path[] = "/tmp/pocketshelf-handoff-XXXXXX";
  int handoff_fd = mkstemp(handoff_path);
  assert(handoff_fd >= 0);
  assert(!register_book(handoff_path) && mock_ui.handoff_count == 0);
  assert(write(handoff_fd, "book", 4) == 4);
  close(handoff_fd);
  assert(register_book(handoff_path) && mock_ui.handoff_count == 1);
  memset(&app->foreground.job, 0, sizeof(app->foreground.job));
  app->foreground.job.type = DOWNLOAD;
  app->foreground.job.done = 1;
  app->foreground.job.ok = 0;
  app->foreground.active = 1;
  assert(
      !pthread_create(&app->foreground.thread, NULL, completed_worker, NULL));
  jobs_poll(app);
  assert(mock_ui.handoff_count == 1 && app->notice_is_error);
  app->foreground.job.ok = 1;
  snprintf(app->foreground.job.path, sizeof(app->foreground.job.path), "%s",
           handoff_path);
  app->foreground.active = 1;
  assert(
      !pthread_create(&app->foreground.thread, NULL, completed_worker, NULL));
  jobs_poll(app);
  assert(mock_ui.handoff_count == 2 && !app->notice_is_error &&
         !strcmp(app->opened, handoff_path));
  app->opened[0] = 0;
  unlink(handoff_path);
  assert(!register_book(handoff_path) && mock_ui.handoff_count == 2);
  app->screen = DETAIL;
  strcpy(app->selected.format, "mobi");
  strcpy(app->selected.title, "1984");
  memset(app->selected.description, 'a', sizeof(app->selected.description) - 1);
  app->selected.description[sizeof(app->selected.description) - 1] = 0;
  app->detail.page = app->detail.tab = 0;
  app->detail.offsets[0] = 0;
  app_render(app);
  assert(strstr(mock_ui.drawn, "Download mobi") && app->detail.more);
  size_t offset = app->detail.offsets[1];
  assert(offset > 0);
  app_action(app, ACTION_TEXT_NEXT);
  assert(app->detail.page == 1 && app->detail.offsets[2] > offset);
  app_action(app, ACTION_TEXT_PREVIOUS);
  assert(app->detail.page == 0);
  app_action(app, ACTION_INFO);
  assert(app->detail.tab == 1 && app->detail.page == 0);
  app_action(app, ACTION_BACK);
  assert(app->screen == LIST);
}
