#ifndef POCKETSHELF_SRC_APP_STATE_H
#define POCKETSHELF_SRC_APP_STATE_H
/* Private application state shared by app controllers and screen renderers.
   Workers access only their Job snapshot; everything else is UI-thread-owned.
 */
#include "app/actions.h"
#include "app/app.h"
#include "core/models.h"
#include "storage/downloads.h"
#include "ui/widgets.h"
#include <dirent.h>
#include <pthread.h>
enum { HOME, LIST, DETAIL, SETTINGS, LOCAL, SIGNIN, SIGNIN_ERROR };
enum { LOGIN = 1, SEARCH, DOWNLOAD, DETAILS, COVER };
enum { EDIT_QUERY, EDIT_BASE, EDIT_EMAIL, EDIT_PASSWORD };
typedef struct {
  int type, done, ok;
  Account account;
  Book book;
  char email[256], password[256], error[512], path[PATH_CAP];
  Transfer transfer;
} Job;
typedef struct {
  int type, done, ok, page, popular;
  Account account;
  Results results;
  Book book;
  char query[512], format[16], error[512];
  Transfer transfer;
} BrowseJob;
typedef struct {
  BookSummary summary;
  int cover_attempted;
} BrowseBook;
typedef struct {
  BrowseBook *books;
  int count, capacity, offset, page, popular, active, discard, more, failed;
  BrowseJob job;
  pthread_t thread;
} Browser;
typedef struct {
  Job job;
  pthread_t thread;
  int active, last_percent;
} Foreground;
typedef struct {
  int tab, page, more;
  size_t offsets[2048];
} Detail;
typedef struct {
  int down, y, offset, dragged;
} Gesture;
struct App {
  Account account;
  Book selected;
  Browser browser;
  Foreground foreground;
  Detail detail;
  Gesture gesture;
  Ui ui;
  LocalBooks downloads;
  DIR *repair_dir;
  pthread_mutex_t lock;
  int screen, edit_kind, local_scroll, keyboard_active;
  int notice_is_error, format_index, scroll_update;
  char query[512], format[16], email[256], edit[512];
  char notice[512], opened[PATH_CAP], password[256];
};

static inline int signed_in(const App *app) {
  return app->account.user[0] && app->account.key[0];
}
#endif
