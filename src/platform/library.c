#include "platform/library.h"
#include "app/state.h"
#include "platform/runtime.h"
#include "storage/paths.h"
#include <stdio.h>
#include <sys/stat.h>
static int saved_book(const char *path) {
  struct stat st;
  return path && *path && stat(path, &st) == 0 && S_ISREG(st.st_mode) &&
         st.st_size > 0;
}

int register_book(const char *path) {
  if (!saved_book(path))
    return 0;
  /* BookPreparing/BookReady are reader lifecycle calls: both associate the
     current task with the book. Calling them here makes OpenBook find
     PocketShelf itself. Ask the firmware scanner to index saved files instead. */
  int scanner = FindTaskByAppName("scanner.app");
  int ok;
  if (scanner > 0)
    ok = SendEventTo(scanner, EVT_STARTSCAN, 0, 0) > 0;
  else {
    const char *args[] = {SYSTEMSCANNER, NULL};
    ok = NewTask(SYSTEMSCANNER, args, "scanner.app", "Library scanner", NULL,
                 TASK_BACKGROUND) > 0;
  }
  fprintf(stderr, "[pocketshelf] library_scan_requested=%d\n", ok);
  fflush(stderr);
  return ok;
}

int open_book(const char *path) {
  if (!saved_book(path)) {
    fprintf(stderr, "[pocketshelf] reader_open_missing_file=1\n");
    return 0;
  }
  int result = OpenBook(path, "r", 0);
  fprintf(stderr, "[pocketshelf] reader_open_result=%d\n", result);
  fflush(stderr);
  return result >= 0;
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
    if (saved_book(path)) {
      /* One native scan covers the entire folder, including older downloads. */
      int ok = register_book(path);
      closedir(app->repair_dir);
      app->repair_dir = NULL;
      if (!ok)
        return; /* Leave the marker absent so the next launch retries. */
      break;
    }
  }
  if (app->repair_dir)
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
