#define _POSIX_C_SOURCE 200809L
#include "cJSON.h"
#include "library.h"
#include <ctype.h>
#include <curl/curl.h>
#include <dirent.h>
#include <inkview.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define DATA_DIR "/mnt/ext1/system/config/pocketshelf"
#define COVER_DIR DATA_DIR "/covers"
#define CONFIG_FILE DATA_DIR "/account.json"
#define BOOK_DIR "/mnt/ext1/Books/PocketShelf"
#define MAX_LOCAL 256
#define COLOR_ERROR 0x00D00000

enum { HOME, LIST, DETAIL, SETTINGS, LOCAL, SIGNIN, SIGNIN_ERROR };
enum { LOGIN = 1, SEARCH, DOWNLOAD, DETAILS };
enum { EDIT_QUERY, EDIT_BASE, EDIT_EMAIL, EDIT_PASSWORD };
typedef struct {
  int x, y, w, h, action;
} Button;
typedef struct {
  int type, done, ok, page, popular;
  Account account;
  Results results;
  Book book;
  char query[512], format[16], email[256], password[256], error[512],
      path[PATH_CAP];
  Transfer transfer;
} Job;
static Account account = {.base = "https://z-library.sk"};
static Results results;
static Book selected;
static Job job;
static pthread_t worker;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int screen = SIGNIN, busy = 0, page = 1, popular = 0, edit_kind = 0,
           last_percent = -1, local_page = 0, local_count = 0,
           keyboard_active = 0;
static char query[512] = "", format[16] = "", email[256] = "", edit[512],
            notice[512] = "", opened[PATH_CAP] = "", password[256] = "";
static char local_names[MAX_LOCAL][256];
static Button buttons[32];
static int button_count;
static DIR *repair_dir;
#define INDEX_REPAIR_MARKER DATA_DIR "/library-handoff-v1"
static int notice_is_error, format_index, detail_tab, detail_page, detail_more;
static size_t detail_offsets[2048];
static ifont *font, *small, *large;
static void render(void);
static void poll_job(void);
static void start_job(int type, int next_page, int browse);
static int signed_in(void) { return account.user[0] && account.key[0]; }
static int sx(int x) { return x * ScreenWidth() / 600; }
static int sy(int y) { return y * ScreenHeight() / 800; }
static void text_color(int x, int y, int w, int h, const char *s, ifont *f,
                       int color) {
  SetFont(f, color);
  DrawTextRect(sx(x), sy(y), sx(w), sy(h), s, ALIGN_LEFT | VALIGN_TOP);
}
static void text_at(int x, int y, int w, int h, const char *s, ifont *f) {
  text_color(x, y, w, h, s, f, BLACK);
}
static void button(int x, int y, int w, int h, const char *label, int action) {
  DrawRect(sx(x), sy(y), sx(w), sy(h), BLACK);
  text_at(x + 10, y + 10, w - 20, h - 14, label, font);
  if (button_count < 32)
    buttons[button_count++] = (Button){x, y, w, h, action};
}
static void draw_cover(const Book *b, int x, int y, int w, int h) {
  ibitmap *image =
      b->cover_path[0] ? LoadImageToFormat(b->cover_path, kFmtRGB24) : NULL;
  if (image && image->width > 0 && image->height > 0) {
    int width = sx(w), height = sy(h);
    if ((long long)image->width * height > (long long)image->height * width)
      height = (long long)image->height * width / image->width;
    else
      width = (long long)image->width * height / image->height;
    DrawBitmapRect(sx(x) + (sx(w) - width) / 2, sy(y) + (sy(h) - height) / 2,
                   width, height, image, STRETCH);
  } else {
    DrawRect(sx(x), sy(y), sx(w), sy(h), 0x888888);
    text_at(x + 4, y + h / 3, w - 8, h / 2, "No cover", small);
  }
  free(image);
}
/* Width-measured UTF-8 wrapping, with stable offsets for backwards paging. */
static void detail_text(void) {
  const char *body = detail_tab ? selected.metadata : selected.description;
  if (!*body)
    body = detail_tab ? "No additional metadata supplied."
                      : "No description supplied for this edition.";
  size_t at = detail_offsets[detail_page], length = strlen(body);
  SetFont(font, BLACK);
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
    text_at(26, 346 + row * 26, 548, 27, line, font);
    at = end;
    if (body[at] == '\n' || body[at] == ' ')
      at++;
  }
  detail_more = at < length && detail_page + 1 < (int)(sizeof(detail_offsets) /
                                                       sizeof(*detail_offsets));
  if (detail_more)
    detail_offsets[detail_page + 1] = at;
  if (detail_page > 0)
    button(24, 588, 170, 48, "Previous", 44);
  char page_label[48];
  snprintf(page_label, sizeof(page_label), "Page %d", detail_page + 1);
  text_at(230, 600, 140, 30, page_label, small);
  if (detail_more)
    button(406, 588, 170, 48, "Next", 45);
}
static void wipe(void *p, size_t n) {
  volatile unsigned char *s = p;
  while (n--)
    *s++ = 0;
}
static int save_account(void) {
  cJSON *j = cJSON_CreateObject();
  if (!j)
    return 0;
  cJSON_AddStringToObject(j, "base", account.base);
  cJSON_AddStringToObject(j, "user", account.user);
  cJSON_AddStringToObject(j, "key", account.key);
  cJSON_AddStringToObject(j, "email", account.email);
  char *json = cJSON_PrintUnformatted(j);
  cJSON_Delete(j);
  if (!json)
    return 0;
  FILE *f = fopen(CONFIG_FILE ".tmp", "w");
  int ok = 0;
  if (f) {
    chmod(CONFIG_FILE ".tmp", 0600);
    ok = fputs(json, f) >= 0;
    if (fclose(f) != 0)
      ok = 0;
    if (ok)
      ok = rename(CONFIG_FILE ".tmp", CONFIG_FILE) == 0;
  }
  if (!ok)
    unlink(CONFIG_FILE ".tmp");
  free(json);
  return ok;
}
static void load_account(void) {
  FILE *f = fopen(CONFIG_FILE, "r");
  if (!f)
    return;
  char data[2048];
  size_t n = fread(data, 1, sizeof(data) - 1, f);
  fclose(f);
  data[n] = 0;
  cJSON *j = cJSON_Parse(data);
  if (!j)
    return;
  cJSON *base = cJSON_GetObjectItemCaseSensitive(j, "base"),
        *user = cJSON_GetObjectItemCaseSensitive(j, "user"),
        *key = cJSON_GetObjectItemCaseSensitive(j, "key"),
        *saved_email = cJSON_GetObjectItemCaseSensitive(j, "email");
  if (cJSON_IsString(base) && valid_base(base->valuestring)) {
    snprintf(account.base, sizeof(account.base), "%s", base->valuestring);
    if (cJSON_IsString(user))
      snprintf(account.user, sizeof(account.user), "%s", user->valuestring);
    if (cJSON_IsString(key))
      snprintf(account.key, sizeof(account.key), "%s", key->valuestring);
  }
  if (cJSON_IsString(saved_email)) {
    snprintf(account.email, sizeof(account.email), "%s",
             saved_email->valuestring);
    snprintf(email, sizeof(email), "%s", saved_email->valuestring);
  }
  cJSON_Delete(j);
}
static int compare_names(const void *a, const void *b) { return strcmp(a, b); }
static void read_local(void) {
  local_count = 0;
  DIR *dir = opendir(BOOK_DIR);
  if (!dir)
    return;
  struct dirent *e;
  while ((e = readdir(dir)) && local_count < MAX_LOCAL) {
    const char *ext = strrchr(e->d_name, '.');
    if (ext && !strcmp(ext, ".zip")) {
      const char *p = ext;
      while (p > e->d_name && p[-1] != '.')
        p--;
      if (p > e->d_name)
        ext = p - 1;
    }
    if (e->d_name[0] == '.' || !ext || !supported_format(ext + 1))
      continue;
    char path[PATH_CAP];
    struct stat st;
    snprintf(path, sizeof(path), "%s/%s", BOOK_DIR, e->d_name);
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode))
      continue;
    snprintf(local_names[local_count++], 256, "%s", e->d_name);
  }
  closedir(dir);
  qsort(local_names, local_count, sizeof(local_names[0]), compare_names);
}
/* Called only on the InkView thread, after the download is complete. */
static int register_book(const char *path) {
  struct stat st;
  if (stat(path, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size <= 0)
    return 0;
  BookPreparing(path);
  BookReady(path);
  fprintf(stderr, "[pocketshelf] library_handoff_requested=1\n");
  fflush(stderr);
  return 1;
}
static void repair_library(void) {
  if (!repair_dir)
    return;
  if (busy || keyboard_active) {
    SetHardTimer("library-repair", repair_library, 1000);
    return;
  }
  struct dirent *e;
  while ((e = readdir(repair_dir))) {
    if (e->d_name[0] == '.')
      continue;
    char path[PATH_CAP];
    if (snprintf(path, sizeof(path), "%s/%s", BOOK_DIR, e->d_name) >=
        (int)sizeof(path))
      continue;
    if (register_book(path)) {
      /* BookReady posts to the native library. Space announcements out so
         the firmware can consume each path before the next notification. */
      SetHardTimer("library-repair", repair_library, 1000);
      return;
    }
  }
  closedir(repair_dir);
  repair_dir = NULL;
  FILE *marker = fopen(INDEX_REPAIR_MARKER, "w");
  if (marker)
    fclose(marker);
}
static void start_library_repair(void) {
  if (repair_dir)
    return;
  repair_dir = opendir(BOOK_DIR);
  if (repair_dir)
    SetHardTimer("library-repair", repair_library, 1000);
}
static void book_overview(const Book *b, char *out, size_t cap) {
  const char *fields[] = {b->author, b->format, b->language};
  if (!cap)
    return;
  out[0] = 0;
  for (size_t i = 0; i < sizeof(fields) / sizeof(*fields); i++) {
    const char *value = fields[i];
    while (isspace((unsigned char)*value))
      value++;
    size_t length = strlen(value);
    while (length && isspace((unsigned char)value[length - 1]))
      length--;
    if (!length)
      continue;
    size_t used = strlen(out);
    snprintf(out + used, cap - used, "%s%.*s", used ? " | " : "", (int)length,
             value);
  }
}
static void render(void) {
  if (!signed_in() && screen != SIGNIN && screen != SIGNIN_ERROR)
    screen = SIGNIN;
  ClearScreen();
  button_count = 0;
  if (!busy && (screen == SETTINGS || screen == LIST || screen == DETAIL ||
                screen == LOCAL))
    button(24, 18, 120, 48, "Back", 1);
  else
    text_at(24, 20, 240, 48, "PocketShelf", large);
  if (signed_in()) {
    char label[280];
    if (account.email[0])
      snprintf(label, sizeof(label), "%s", account.email);
    else
      snprintf(label, sizeof(label), "User %s", account.user);
    SetFont(small, BLACK);
    DrawTextRect(sx(280), sy(20), sx(296), sy(52), label,
                 ALIGN_RIGHT | VALIGN_TOP);
  }
  DrawLine(sx(24), sy(82), sx(576), sy(82), BLACK);
  if (busy) {
    text_at(24, 135, 550, 100,
            job.type == LOGIN      ? "Signing in..."
            : job.type == DOWNLOAD ? "Downloading your book..."
                                   : "Loading books...",
            large);
    text_at(24, 250, 550, 150,
            "Keep Wi-Fi connected. You can cancel without leaving an "
            "incomplete book in your library.",
            font);
    char p[64];
    int percent = transfer_percent(&job.transfer);
    snprintf(p, sizeof(p), percent > 0 ? "Progress: %d%%" : "Connecting...",
             percent);
    text_at(24, 410, 550, 40, p, font);
    button(24, 495, 552, 60, "Cancel", 2);
  } else if (screen == HOME) {
    text_at(24, 110, 552, 70, "Find your next book.", large);
    text_at(24, 178, 552, 68,
            "Download books and open them with PocketBook's built-in reader.",
            font);
    button(24, 276, 552, 66, "Search the library", 10);
    button(24, 362, 552, 66, "Browse popular books", 11);
    button(24, 448, 552, 66, "Downloaded books", 12);
    button(24, 534, 552, 66, "Account & settings", 13);
    text_at(24, 624, 552, 36, "Account connected", small);
  } else if (screen == SETTINGS) {
    text_at(24, 105, 552, 38, "Account & settings", large);
    text_at(24, 163, 552, 52, account.base, small);
    button(24, 220, 552, 55, "Change library address", 20);
    text_at(24, 292, 552, 55, account.email[0] ? account.email : account.user,
            small);
    char filter[100];
    snprintf(filter, sizeof(filter), "Search format: %s (tap to change)",
             *format ? format : "All formats");
    button(24, 364, 552, 55, filter, 22);
    button(24, 436, 552, 55, "Sign out & forget session", 23);
    button(24, 508, 552, 55, "Refresh downloads in Library", 27);
    text_at(24, 584, 552, 116,
            "Your password is used only to sign in. A session token is saved "
            "on this device. Changing the address signs you out.",
            small);
  } else if (screen == SIGNIN_ERROR) {
    FillArea(sx(24), sy(108), sx(552), sy(5), COLOR_ERROR);
    DrawRect(sx(24), sy(108), sx(552), sy(416), COLOR_ERROR);
    text_color(40, 130, 520, 64, "Sign-in failed", large, COLOR_ERROR);
    text_color(40, 223, 520, 280, notice, font, COLOR_ERROR);
    button(24, 548, 552, 66, "Back to sign-in", 1);
    text_at(24, 646, 552, 54,
            "Your entries are kept so you can correct them or retry.", small);
  } else if (screen == SIGNIN) {
    text_at(24, 108, 552, 48, "Sign in to Z-Library", large);
    text_at(24, 170, 552, 26, "Library address", small);
    button(24, 202, 552, 54, account.base, 20);
    text_at(24, 282, 552, 30, "Email", small);
    button(24, 314, 552, 64, email[0] ? email : "Enter your email", 24);
    text_at(24, 402, 552, 30, "Password", small);
    button(24, 434, 552, 64, password[0] ? "********" : "Enter your password",
           25);
    button(24, 548, 552, 66, "Sign in", 26);
    text_at(24, 646, 552, 54,
            "Tap a field to edit it. Your password is not saved.", small);
  } else if (screen == LIST) {
    char title[600];
    snprintf(title, sizeof(title), "%s  /  page %d",
             popular ? "Popular books" : query, page);
    text_at(24, 103, 552, 46, title, font);
    if (!results.count)
      text_at(24, 210, 552, 100,
              "No books found. Try another search or format.", font);
    for (int i = 0; i < results.count; i++) {
      int y = 154 + i * 120;
      Book *b = &results.books[i];
      DrawLine(sx(24), sy(y + 116), sx(576), sy(y + 116), BLACK);
      draw_cover(b, 26, y + 4, 66, 104);
      text_at(108, y + 2, 466, 54, b->title, font);
      char meta[400];
      book_overview(b, meta, sizeof(meta));
      text_at(108, y + 62, 466, 48, meta, small);
      buttons[button_count++] = (Button){24, y, 552, 120, 100 + i};
    }
    if (page > 1)
      button(24, 650, 160, 52, "Previous", 30);
    button(204, 650, 192, 52, "New search", 10);
    if (results.has_more)
      button(416, 650, 160, 52, "Next", 31);
  } else if (screen == DETAIL) {
    draw_cover(&selected, 24, 104, 104, 158);
    text_at(148, 104, 428, 94, selected.title, large);
    text_at(148, 204, 428, 57, selected.author, small);
    button(24, 282, 266, 48, detail_tab ? "Description" : "[Description]", 42);
    button(310, 282, 266, 48, detail_tab ? "[Book info]" : "Book info", 43);
    detail_text();
    if (opened[0])
      button(24, 650, 552, 56, "Open in PocketBook reader", 41);
    else if (supported_format(selected.format)) {
      char label[80];
      snprintf(label, sizeof(label), "Download %s%s%s", selected.format,
               selected.size[0] ? " - " : "", selected.size);
      button(24, 650, 552, 56, label, 40);
    } else
      text_at(24, 650, 552, 56, "This format is not supported by PocketBook.",
              small);
  } else if (screen == LOCAL) {
    text_at(24, 105, 552, 40, "Downloaded books", large);
    if (!local_count)
      text_at(24, 210, 552, 110, "Your downloaded books will appear here.",
              font);
    for (int i = 0; i < 8 && local_page * 8 + i < local_count; i++) {
      int y = 154 + i * 59;
      button(24, y, 552, 55, local_names[local_page * 8 + i], 200 + i);
    }
    if (local_page > 0)
      button(24, 650, 200, 52, "Previous", 32);
    if ((local_page + 1) * 8 < local_count)
      button(376, 650, 200, 52, "Next", 33);
  }
  if (notice[0] && !busy && screen != SIGNIN_ERROR)
    text_color(24, 718, 552, 78, notice, small,
               notice_is_error ? COLOR_ERROR : BLACK);
  FullUpdate();
}
static void *run_job(void *unused) {
  (void)unused;
  if (job.type == LOGIN)
    job.ok = library_login(&job.account, job.email, job.password, &job.transfer,
                           job.error, sizeof(job.error));
  else if (job.type == SEARCH) {
    job.ok = library_search(&job.account, job.query, job.format, job.page,
                            job.popular, &job.results, &job.transfer, job.error,
                            sizeof(job.error));
    if (job.ok)
      for (int i = 0; i < job.results.count &&
                      !__sync_fetch_and_add(&job.transfer.cancel, 0);
           i++)
        library_cover(&job.account, &job.results.books[i], COVER_DIR,
                      &job.transfer);
  } else if (job.type == DETAILS) {
    job.ok = library_details(&job.account, &job.book, &job.transfer, job.error,
                             sizeof(job.error));
    if (job.ok)
      library_cover(&job.account, &job.book, COVER_DIR, &job.transfer);
  } else
    job.ok = library_download(&job.account, &job.book, BOOK_DIR, job.path,
                              sizeof(job.path), &job.transfer, job.error,
                              sizeof(job.error));
  wipe(job.password, sizeof(job.password));
  fprintf(stderr, "[pocketshelf] job_finished type=%d success=%d\n", job.type,
          job.ok);
  fflush(stderr);
  pthread_mutex_lock(&lock);
  job.done = 1;
  pthread_mutex_unlock(&lock);
  return NULL;
}
static void poll_job(void) {
  pthread_mutex_lock(&lock);
  int done = job.done;
  pthread_mutex_unlock(&lock);
  if (!done) {
    int p = transfer_percent(&job.transfer);
    if (job.type == DOWNLOAD && (p == 100 || p / 10 != last_percent / 10) &&
        p != last_percent) {
      last_percent = p;
      render();
    }
    SetHardTimer("pocketshelf", poll_job, 500);
    return;
  }
  pthread_join(worker, NULL);
  busy = 0;

  notice_is_error = !job.ok;
  if (!job.ok) {
    snprintf(notice, sizeof(notice), "%s",
             job.error[0] ? job.error : "Operation failed.");
    if (job.type == LOGIN)
      screen = SIGNIN_ERROR;
  } else if (job.type == LOGIN) {
    wipe(password, sizeof(password));
    account = job.account;
    snprintf(account.email, sizeof(account.email), "%s", job.email);
    screen = HOME;
    snprintf(
        notice, sizeof(notice), "%s",
        save_account()
            ? "Signed in. Ready to find books."
            : "Signed in for this session; could not save account settings.");
  } else if (job.type == SEARCH) {
    results = job.results;
    page = job.page;
    popular = job.popular;
    screen = LIST;
    notice[0] = 0;
  } else if (job.type == DETAILS) {
    selected = job.book;
    screen = DETAIL;
    notice[0] = 0;
  } else {
    snprintf(opened, sizeof(opened), "%s", job.path);
    int registered = register_book(job.path);
    notice_is_error = !registered;
    snprintf(notice, sizeof(notice), "%s",
             registered
                 ? "Book saved. Library update requested. Tap Open to read."
                 : "Book saved, but could not hand it to the Library. Try "
                   "Refresh downloads in settings.");
    screen = DETAIL;
  }
  render();
}
static void launch_job(void) {
  busy = 1;
  last_percent = -1;
  notice[0] = 0;
  render();
  fprintf(stderr, "[pocketshelf] job_started type=%d\n", job.type);
  int connection = NetConnect2(NULL, 1);
  fprintf(stderr, "[pocketshelf] connection_result=%d\n", connection);
  fflush(stderr);
  if (pthread_create(&worker, NULL, run_job, NULL) != 0) {
    busy = 0;
    wipe(job.password, sizeof(job.password));
    notice_is_error = 1;
    snprintf(notice, sizeof(notice), "Could not start the network task.");
    if (job.type == LOGIN)
      screen = SIGNIN_ERROR;
    fprintf(stderr, "[pocketshelf] worker_start_failed\n");
    fflush(stderr);
    render();
    return;
  }
  SetHardTimer("pocketshelf", poll_job, 500);
}
static void start_job(int type, int next_page, int browse) {
  if (busy)
    return;
  if (!signed_in() && type != LOGIN) {
    screen = SIGNIN;
    snprintf(notice, sizeof(notice), "Sign in before browsing or downloading.");
    render();
    return;
  }
  memset(&job, 0, sizeof(job));
  job.type = type;
  job.account = account;
  job.page = next_page;
  job.popular = browse;
  job.book = selected;
  snprintf(job.query, sizeof(job.query), "%s", query);
  snprintf(job.format, sizeof(job.format), "%s", format);
  launch_job();
}
static void finish_keyboard(void) {
  keyboard_active = 0;
  render();
}
static void search_after_keyboard(void) {
  keyboard_active = 0;
  if (query[0])
    start_job(SEARCH, 1, 0);
  else
    render();
}
static void keyboard_done(char *value) {
  /* InkView still owns the modal during this callback. Only update state;
     redraw or start another operation after it has returned and closed. */
  if (value) {
    if (edit_kind == EDIT_QUERY) {
      snprintf(query, sizeof(query), "%s", value);
    } else if (edit_kind == EDIT_BASE) {
      size_t n = strlen(value);
      while (n && value[n - 1] == '/')
        value[--n] = 0;
      if (!valid_base(value)) {
        notice_is_error = 1;
        snprintf(notice, sizeof(notice),
                 "Use https:// followed by a hostname, without a path.");
      } else {
        snprintf(account.base, sizeof(account.base), "%s", value);
        wipe(account.user, sizeof(account.user));
        wipe(account.key, sizeof(account.key));
        snprintf(notice, sizeof(notice), "%s",
                 save_account() ? "Address saved. Sign in to this library."
                                : "Could not save settings.");
      }
    } else if (edit_kind == EDIT_EMAIL) {
      snprintf(email, sizeof(email), "%s", value);
    } else if (edit_kind == EDIT_PASSWORD) {
      snprintf(password, sizeof(password), "%s", value);
    }
  }
  int search = value && edit_kind == EDIT_QUERY;
  wipe(edit, sizeof(edit));
  SetHardTimer("pocketshelf-keyboard",
               search ? search_after_keyboard : finish_keyboard, 100);
}
static void edit_field(int kind, const char *title, const char *value,
                       int flags) {
  edit_kind = kind;
  snprintf(edit, sizeof(edit), "%s", value);
  keyboard_active = 1;
  OpenKeyboard(title, edit, kind == EDIT_QUERY ? 511 : 255, flags,
               keyboard_done);
}

static void act(int action) {
  if (keyboard_active)
    return;
  if (busy) {
    if (action == 1 || action == 2) {
      transfer_cancel(&job.transfer);
    }
    return;
  }
  if (!signed_in() && action != 1 && action != 20 && action != 24 &&
      action != 25 && action != 26) {
    screen = SIGNIN;
    render();
    return;
  }
  notice[0] = 0;
  notice_is_error = 0;
  if (action == 1) {
    if (screen == HOME || screen == SIGNIN) {
      wipe(password, sizeof(password));
      CloseApp();
      return;
    }
    if (screen == SIGNIN_ERROR) {
      screen = SIGNIN;
    } else
      screen = screen == DETAIL ? LIST : HOME;
  } else if (action == 10) {
    edit_field(EDIT_QUERY, "Title, author or ISBN", query, 0);
    return;
  } else if (action == 11) {
    start_job(SEARCH, 1, 1);
    return;
  } else if (action == 12) {
    read_local();
    local_page = 0;
    screen = LOCAL;
  } else if (action == 13)
    screen = SETTINGS;
  else if (action == 20) {
    edit_field(EDIT_BASE, "HTTPS library address", account.base, 0);
    return;
  } else if (action == 24 && screen == SIGNIN) {
    edit_field(EDIT_EMAIL, "Email", email, 0);
    return;
  } else if (action == 25 && screen == SIGNIN) {
    edit_field(EDIT_PASSWORD, "Password", password, KBD_PASSWORD);
    return;
  } else if (action == 26 && screen == SIGNIN) {
    if (!email[0] || !password[0]) {
      notice_is_error = 1;
      snprintf(notice, sizeof(notice), "Enter both your email and password.");
    } else {
      memset(&job, 0, sizeof(job));
      job.type = LOGIN;
      job.account = account;
      snprintf(job.email, sizeof(job.email), "%s", email);
      snprintf(job.password, sizeof(job.password), "%s", password);
      launch_job();
      return;
    }
  } else if (action == 22) {
    if (!format_option(++format_index))
      format_index = 0;
    snprintf(format, sizeof(format), "%s", format_option(format_index));
  } else if (action == 27) {
    start_library_repair();
    snprintf(notice, sizeof(notice),
             "Updating downloaded books in the PocketBook Library.");
  } else if (action == 23) {
    wipe(account.user, sizeof(account.user));
    wipe(account.key, sizeof(account.key));
    wipe(account.email, sizeof(account.email));
    wipe(password, sizeof(password));
    screen = SIGNIN;
    wipe(&job, sizeof(job));
    unlink(CONFIG_FILE);
    unlink(CONFIG_FILE ".tmp");
    snprintf(notice, sizeof(notice), "Signed out. Saved session removed.");
  } else if (action == 30 && page > 1) {
    start_job(SEARCH, page - 1, popular);
    return;
  } else if (action == 31 && results.has_more) {
    start_job(SEARCH, page + 1, popular);
    return;
  } else if (action == 32 && local_page > 0)
    local_page--;
  else if (action == 33 && (local_page + 1) * 8 < local_count)
    local_page++;
  else if (action == 40) {
    start_job(DOWNLOAD, page, popular);
    return;
  } else if (action == 42 || action == 43) {
    detail_tab = action == 43;
    detail_page = 0;
    detail_offsets[0] = 0;
  } else if (action == 44 && detail_page > 0)
    detail_page--;
  else if (action == 45 && detail_more)
    detail_page++;
  else if (action == 41 && opened[0]) {
    OpenBook(opened, "r", 0);
    return;
  } else if (action >= 100 && action < 100 + results.count) {
    selected = results.books[action - 100];
    opened[0] = 0;
    screen = DETAIL;
    detail_tab = detail_page = 0;
    detail_offsets[0] = 0;
    start_job(DETAILS, page, popular);
    return;
  } else if (action >= 200 && action < 208 &&
             local_page * 8 + action - 200 < local_count) {
    char p[PATH_CAP];
    snprintf(p, sizeof(p), "%s/%s", BOOK_DIR,
             local_names[local_page * 8 + action - 200]);
    OpenBook(p, "r", 0);
    return;
  }
  render();
}
static int handler(int event, int p1, int p2) {
  if (event == EVT_INIT) {
    icanvas *canvas = GetCanvas();
    fprintf(stderr, "[pocketshelf] canvas_depth=%d\n",
            canvas ? canvas->depth : 0);
    fflush(stderr);
    mkdir("/mnt/ext1/Books", 0755);
    mkdir(BOOK_DIR, 0755);
    mkdir(DATA_DIR, 0700);
    mkdir(COVER_DIR, 0700);
    load_account();
    screen = signed_in() ? HOME : SIGNIN;
    font = OpenFont("DejaVuSans", sx(20), 1);
    small = OpenFont("DejaVuSans", sx(16), 1);
    large = OpenFont("DejaVuSans", sx(29), 1);
    if (access(INDEX_REPAIR_MARKER, F_OK) != 0)
      start_library_repair();
  } else if (event == EVT_SHOW) {
    if (!keyboard_active)
      render();
  } else if (event == EVT_POINTERUP) {
    int x = p1 * 600 / ScreenWidth(), y = p2 * 800 / ScreenHeight();
    for (int i = 0; i < button_count; i++) {
      Button b = buttons[i];
      if (x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h) {
        act(b.action);
        break;
      }
    }
  } else if (event == EVT_KEYPRESS) {
    if (p1 == IV_KEY_BACK)
      act(1);
    else if (!busy && screen == LIST && p1 == IV_KEY_NEXT)
      act(31);
    else if (!busy && screen == LIST && p1 == IV_KEY_PREV)
      act(30);
    else if (!busy && screen == DETAIL && p1 == IV_KEY_NEXT)
      act(45);
    else if (!busy && screen == DETAIL && p1 == IV_KEY_PREV)
      act(44);
    else if (!busy && screen == LOCAL && p1 == IV_KEY_NEXT)
      act(33);
    else if (!busy && screen == LOCAL && p1 == IV_KEY_PREV)
      act(32);
  } else if (event == EVT_EXIT) {
    ClearTimer(poll_job);
    ClearTimer(repair_library);
    if (repair_dir) {
      closedir(repair_dir);
      repair_dir = NULL;
    }
    ClearTimer(finish_keyboard);
    ClearTimer(search_after_keyboard);
    wipe(password, sizeof(password));
    wipe(edit, sizeof(edit));
    if (busy) {
      transfer_cancel(&job.transfer);
      pthread_join(worker, NULL);
    }
    CloseFont(font);
    CloseFont(small);
    CloseFont(large);
    wipe(&account, sizeof(account));
    wipe(&job, sizeof(job));
  }
  return 0;
}
int main(void) {
  umask(0077);
  curl_global_init(CURL_GLOBAL_DEFAULT);
  InitInkview(TASK_MAKEACTIVE | TASK_FB_RGB24);
  InkViewMain(handler);
  curl_global_cleanup();
  return 0;
}
