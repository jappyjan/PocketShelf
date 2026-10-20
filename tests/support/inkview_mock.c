#include "support/app_test.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
MockInkView mock_ui;
int FindTaskByAppName(const char *name) {
  assert(!strcmp(name, "scanner.app"));
  assert(pthread_equal(pthread_self(), mock_ui.ui_thread));
  return mock_ui.scanner_task;
}

int SendEventTo(int task, int event, int p1, int p2) {
  assert(task > 0 && task == mock_ui.scanner_task);
  assert(event == EVT_STARTSCAN && !p1 && !p2);
  assert(pthread_equal(pthread_self(), mock_ui.ui_thread));
  mock_ui.scan_requests++;
  return !mock_ui.scan_fail;
}

int NewTask(const char *path, const char *const args[], const char *appname,
            const char *name, const ibitmap *icon, unsigned int flags) {
  assert(!strcmp(path, SYSTEMSCANNER) && !strcmp(args[0], SYSTEMSCANNER));
  assert(!args[1] && !strcmp(appname, "scanner.app") && name && !icon);
  assert(flags == TASK_BACKGROUND);
  assert(pthread_equal(pthread_self(), mock_ui.ui_thread));
  mock_ui.scanner_launches++;
  mock_ui.scan_requests++;
  return mock_ui.scan_fail ? -1 : 42;
}

void BookPreparing(const char *p) {
  assert(p && *p);
  assert(pthread_equal(pthread_self(), mock_ui.ui_thread));
  assert(!mock_ui.handoff_pending);
  mock_ui.handoff_pending = 1;
  snprintf(mock_ui.reader_book, sizeof(mock_ui.reader_book), "%s", p);
}

void BookReady(const char *p) {
  assert(p && *p);
  assert(pthread_equal(pthread_self(), mock_ui.ui_thread));
  assert(mock_ui.handoff_pending);
  mock_ui.handoff_pending = 0;
  mock_ui.handoff_count++;
  /* SDK BookReady also calls SetSubtaskInfo(current_task, 0, title, p).
     OpenBook subsequently finds this task instead of launching a reader. */
  snprintf(mock_ui.reader_book, sizeof(mock_ui.reader_book), "%s", p);
}

void SetFont(const ifont *f, int c) {
  (void)f;
  mock_ui.active_color = c;
}

char *DrawTextRect(int x, int y, int w, int h, const char *s, int f) {
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)f;
  if (x >= sx(280) && y == sy(20) && (f & ALIGN_RIGHT) &&
      !strcmp(s, "reader@example.org"))
    mock_ui.header_identity = 1;
  if (!strcmp(s, "Sign-in failed"))
    mock_ui.error_heading_color = mock_ui.active_color;
  strncat(mock_ui.drawn, s, sizeof(mock_ui.drawn) - strlen(mock_ui.drawn) - 1);
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
  assert(!mock_ui.in_keyboard_callback &&
         "Do not redraw while the keyboard is closing");
  mock_ui.drawn[0] = 0;
  mock_ui.header_identity = 0;
}

void FullUpdate(void) { mock_ui.full_updates++; }

void SetClip(int x, int y, int w, int h) {
  (void)x;
  (void)y;
  (void)w;
  (void)h;
}

void PartialUpdate(int x, int y, int w, int h) {
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  mock_ui.partial_updates++;
}

void InitInkview(int flags) { mock_ui.initialization_flags = flags; }

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

void CloseApp(void) { mock_ui.close_calls++; }

void OpenKeyboard(const char *title, char *buffer, int max, int flags,
                  iv_keyboardhandler cb) {
  (void)title;
  (void)max;
  if (mock_ui.in_keyboard_callback)
    mock_ui.nested_keyboard++;
  mock_ui.keyboard_calls++;
  mock_ui.keyboard_flags = flags;
  mock_ui.keyboard_buffer = buffer;
  mock_ui.callback = cb;
}

void SetHardTimer(const char *name, void (*cb)(void), int ms) {
  (void)name;
  (void)ms;
  mock_ui.pending_timer = cb;
}

void ClearTimer(void (*cb)(void)) {
  if (mock_ui.pending_timer == cb)
    mock_ui.pending_timer = NULL;
}

int NetConnect2(const char *name, int hourglass) {
  (void)name;
  (void)hourglass;
  return 0;
}

int OpenBook(const char *path, const char *p, int flags) {
  (void)p;
  (void)flags;
  mock_ui.open_calls++;
  if (strcmp(path, mock_ui.reader_book) && mock_ui.open_result >= 0)
    mock_ui.reader_launches++;
  return mock_ui.open_result;
}

ifont *OpenFont(const char *name, int size, int aa) {
  (void)name;
  (void)size;
  (void)aa;
  return NULL;
}

void CloseFont(ifont *f) { (void)f; }

void InkViewMain(int (*h)(int, int, int)) { (void)h; }

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
