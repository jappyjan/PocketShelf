#define library_login fixture_login
#define main pocketshelf_main
#include "../src/main.c"
#undef main
#undef library_login
#include <assert.h>
#include <time.h>
static int handoff_count, handoff_pending;
static pthread_t ui_thread;
void BookPreparing(const char *p) {
  assert(p && *p);
  assert(pthread_equal(pthread_self(), ui_thread));
  assert(!handoff_pending);
  handoff_pending = 1;
}
void BookReady(const char *p) {
  assert(p && *p);
  assert(pthread_equal(pthread_self(), ui_thread));
  assert(handoff_pending);
  handoff_pending = 0;
  handoff_count++;
}
static void *completed_worker(void *unused) {
  (void)unused;
  return NULL;
}
static int fixture_success;
int fixture_login(Account *a, const char *e, const char *p, Transfer *t,
                  char *error, size_t cap) {
  (void)a;
  (void)t;
  assert(!strcmp(e, "reader@example.org"));
  assert(!strcmp(p, "secret-test-password"));
  if (fixture_success) {
    snprintf(a->user, sizeof(a->user), "123");
    snprintf(a->key, sizeof(a->key), "fixture-session");
    return 1;
  }
  snprintf(error, cap,
           "The site blocked this request or requires a browser check.");
  return 0;
}
static int in_keyboard_callback, nested_keyboard, keyboard_calls,
    keyboard_flags;
static iv_keyboardhandler callback;
static char *keyboard_buffer;
static void (*pending_timer)(void);
static char drawn[16384];
static int header_identity, close_calls, active_color, error_heading_color;
void SetFont(const ifont *f, int c) {
  (void)f;
  active_color = c;
}
char *DrawTextRect(int x, int y, int w, int h, const char *s, int f) {
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)f;
  if (x >= sx(280) && y == sy(20) && (f & ALIGN_RIGHT) &&
      !strcmp(s, "reader@example.org"))
    header_identity = 1;
  if (!strcmp(s, "Sign-in failed"))
    error_heading_color = active_color;
  strncat(drawn, s, sizeof(drawn) - strlen(drawn) - 1);
  return NULL;
}
void DrawRect(int x, int y, int w, int h, int c) {
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)c;
}
void DrawLine(int x, int y, int w, int h, int c) {
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)c;
}
void ClearScreen(void) {
  assert(!in_keyboard_callback &&
         "Do not redraw while the keyboard is closing");
  drawn[0] = 0;
  header_identity = 0;
}
void FullUpdate(void) {}
static int initialization_flags;
void InitInkview(int flags) { initialization_flags = flags; }
icanvas *GetCanvas(void) {
  static icanvas canvas = {24};
  return &canvas;
}
void FillArea(int x, int y, int w, int h, int color) {
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)color;
}
int ScreenWidth(void) { return 1264; }
int ScreenHeight(void) { return 1680; }
void CloseApp(void) { close_calls++; }
void OpenKeyboard(const char *title, char *buffer, int max, int flags,
                  iv_keyboardhandler cb) {
  (void)title;
  (void)max;
  if (in_keyboard_callback)
    nested_keyboard++;
  keyboard_calls++;
  keyboard_flags = flags;
  keyboard_buffer = buffer;
  callback = cb;
}
void SetHardTimer(const char *name, void (*cb)(void), int ms) {
  (void)name;
  (void)ms;
  pending_timer = cb;
}
void ClearTimer(void (*cb)(void)) {
  if (pending_timer == cb)
    pending_timer = NULL;
}
int NetConnect2(const char *name, int hourglass) {
  (void)name;
  (void)hourglass;
  return 0;
}
int OpenBook(const char *path, const char *p, int flags) {
  (void)path;
  (void)p;
  (void)flags;
  return 0;
}
ifont *OpenFont(const char *name, int size, int aa) {
  (void)name;
  (void)size;
  (void)aa;
  return NULL;
}
void CloseFont(ifont *f) { (void)f; }
void InkViewMain(int (*h)(int, int, int)) { (void)h; }
static void complete_keyboard(const char *value) {
  if (value)
    snprintf(keyboard_buffer, 512, "%s", value);
  in_keyboard_callback = 1;
  callback(value ? keyboard_buffer : NULL);
  in_keyboard_callback = 0;
  int previous_screen = screen;
  handler(EVT_POINTERUP, sx(500), sy(35));
  assert(screen == previous_screen &&
         "Ignore pointer-up events left over from the closing keyboard");
  assert(nested_keyboard == 0 &&
         "Opening the password keyboard from the email keyboard callback "
         "causes modal teardown to close it");
  if (pending_timer) {
    void (*cb)(void) = pending_timer;
    pending_timer = NULL;
    cb();
  }
}
static void tap_back(void) {
  int found = 0;
  Button target = {0};
  for (int i = 0; i < button_count; i++) {
    if (buttons[i].action == 1) {
      target = buttons[i];
      found = 1;
      break;
    }
  }
  assert(found && "Every inner menu page must have a visible Back button");
  handler(EVT_POINTERUP, sx(target.x + target.w / 2),
          sy(target.y + target.h / 2));
}
int main(void) {
  ui_thread = pthread_self();
  screen = HOME;
  render();
  assert(screen == SIGNIN &&
         "Logged-out startup must show only the login form");
  assert(!strstr(drawn, "Search the library"));
  assert(!strstr(drawn, "Browse popular books"));
  assert(!strstr(drawn, "Close"));
  assert(screen == SIGNIN && keyboard_calls == 0);
  assert(strstr(drawn, "Email") && strstr(drawn, "Password"));
  act(26);
  assert(!busy && strstr(notice, "both"));
  act(24);
  assert(keyboard_calls == 1);
  complete_keyboard("reader@example.org");
  assert(!strcmp(email, "reader@example.org"));
  assert(keyboard_calls == 1);
  act(25);
  assert(keyboard_calls == 2);
  assert(keyboard_flags & KBD_PASSWORD);
  complete_keyboard("secret-test-password");
  assert(!busy && "Entering a password must not submit the form");
  assert(strstr(drawn, "secret-test-password") == NULL);
  assert(strstr(drawn, "reader@example.org") != NULL);
  act(25);
  complete_keyboard(NULL);
  assert(!busy);
  act(26);
  assert(password[0] &&
         "Submitting sign-in must retain the form password until success");
  for (int i = 0; busy && i < 200; ++i) {
    struct timespec delay = {0, 1000000};
    nanosleep(&delay, NULL);
    poll_job();
  }
  assert(!busy);
  assert(password[0] && "A rejected login must not erase the password");
  assert(strstr(drawn, "Sign-in failed") &&
         "A login failure must have a prominent result screen");
  assert(strstr(drawn, "browser check"));
  assert(error_heading_color == 0x00D00000 &&
         "Errors must be red on the color screen");
  act(1); /* back to sign-in */
  assert(screen == SIGNIN && password[0]);
  fixture_success = 1;
  act(26);
  for (int i = 0; busy && i < 200; ++i) {
    struct timespec delay = {0, 1000000};
    nanosleep(&delay, NULL);
    poll_job();
  }
  assert(!busy && account.key[0]);
  assert(!password[0] && "Successful sign-in must clear the password");
  assert(screen == HOME);
  assert(strstr(drawn, "Search the library"));
  assert(strstr(drawn, "Browse popular books"));
  assert(strstr(drawn, "reader@example.org"));
  assert(header_identity && "Current user belongs at the top right");
  assert(!strstr(drawn, "Close"));
  assert(!password[0]);
  const int inner_screens[] = {SETTINGS, LIST, LOCAL, DETAIL};
  for (unsigned i = 0; i < sizeof(inner_screens) / sizeof(inner_screens[0]);
       i++) {
    screen = inner_screens[i];
    render();
    assert(header_identity);
    tap_back();
    assert(screen == (inner_screens[i] == DETAIL ? LIST : HOME));
    assert(close_calls == 0);
  }
  screen = HOME;
  render();
  for (int i = 0; i < button_count; i++)
    assert(buttons[i].action != 1);
  act(13);
  act(23);
  assert(screen == SIGNIN);
  assert(!strstr(drawn, "Search the library"));
  act(11);
  assert(screen == SIGNIN && !busy);
  act(10);
  assert(screen == SIGNIN && !keyboard_active);
  handler(EVT_KEYPRESS, IV_KEY_BACK, 0);
  assert(close_calls == 1 && "Physical Back closes the login screen");
  pocketshelf_main();
  assert(initialization_flags & TASK_FB_RGB24);
  strcpy(account.user, "123");
  strcpy(account.key, "fixture-session");
  screen = LIST;
  results.count = 1;
  memset(&results.books[0], 0, sizeof(Book));
  strcpy(results.books[0].title, "Missing metadata");
  strcpy(results.books[0].format, "mobi");
  render();
  assert(!strstr(drawn, " | ") &&
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
  assert(!register_book(handoff_path) && handoff_count == 0);
  assert(write(handoff_fd, "book", 4) == 4);
  close(handoff_fd);
  assert(register_book(handoff_path) && handoff_count == 1);
  memset(&job, 0, sizeof(job));
  job.type = DOWNLOAD;
  job.done = 1;
  job.ok = 0;
  busy = 1;
  assert(!pthread_create(&worker, NULL, completed_worker, NULL));
  poll_job();
  assert(handoff_count == 1 && notice_is_error);
  job.ok = 1;
  snprintf(job.path, sizeof(job.path), "%s", handoff_path);
  busy = 1;
  assert(!pthread_create(&worker, NULL, completed_worker, NULL));
  poll_job();
  assert(handoff_count == 2 && !notice_is_error &&
         !strcmp(opened, handoff_path));
  opened[0] = 0;
  unlink(handoff_path);
  assert(!register_book(handoff_path) && handoff_count == 2);
  screen = DETAIL;
  strcpy(selected.format, "mobi");
  strcpy(selected.title, "1984");
  memset(selected.description, 'a', sizeof(selected.description) - 1);
  selected.description[sizeof(selected.description) - 1] = 0;
  detail_page = detail_tab = 0;
  detail_offsets[0] = 0;
  render();
  assert(strstr(drawn, "Download mobi") && detail_more);
  size_t offset = detail_offsets[1];
  assert(offset > 0);
  act(45);
  assert(detail_page == 1 && detail_offsets[2] > offset);
  act(44);
  assert(detail_page == 0);
  act(43);
  assert(detail_tab == 1 && detail_page == 0);
  act(1);
  assert(screen == LIST);
  puts("Sign-in keyboard, login-only navigation and color regression passed.");
  return 0;
}

ibitmap *LoadImageToFormat(const char *p, int f) {
  (void)p;
  (void)f;
  return NULL;
}
void DrawBitmapRect(int x, int y, int w, int h, const ibitmap *b, int f) {
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)b;
  (void)f;
}
int StringWidth(const char *s) { return (int)strlen(s) * 10; }
