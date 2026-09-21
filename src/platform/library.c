#include "platform/library.h"
#include "app/state.h"
#include "platform/runtime.h"
#include "storage/paths.h"
#include <stdio.h>
#include <sys/stat.h>
int register_book(const char *path) {
  struct stat st;
  if (stat(path, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size <= 0)
    return 0;
  BookPreparing(path);
  BookReady(path);
  fprintf(stderr, "[pocketshelf] library_handoff_requested=1\n");
  fflush(stderr);
  return 1;
}
void repair_library(App *app) {
  if (!app->repair_dir)
    return;
  if (app->foreground.active || app->keyboard_active) {
    runtime_schedule(app, TIMER_REPAIR, 1000);
    return;
  }
  struct dirent *e;
  while ((e = readdir(app->repair_dir))) {
    if (e->d_name[0] == '.')
      continue;
    char path[PATH_CAP];
    if (snprintf(path, sizeof(path), "%s/%s", BOOK_DIR, e->d_name) >=
        (int)sizeof(path))
      continue;
    if (register_book(path)) {
      /* BookReady posts to the native library. Space announcements out so
         the firmware can consume each path before the next notification. */
      runtime_schedule(app, TIMER_REPAIR, 1000);
      return;
    }
  }
  closedir(app->repair_dir);
  app->repair_dir = NULL;
  FILE *marker = fopen(INDEX_REPAIR_MARKER, "w");
  if (marker)
    fclose(marker);
}

void start_library_repair(App *app) {
  if (app->repair_dir)
    return;
  app->repair_dir = opendir(BOOK_DIR);
  if (app->repair_dir)
    runtime_schedule(app, TIMER_REPAIR, 1000);
}

void stop_library_repair(App *app) {
  runtime_clear(TIMER_REPAIR);
  if (app->repair_dir)
    closedir(app->repair_dir);
  app->repair_dir = NULL;
}
