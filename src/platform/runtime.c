#include "platform/runtime.h"
#include "app/browser.h"
#include "app/jobs.h"
#include "app/keyboard.h"
#include "platform/library.h"
#include <curl/curl.h>
#include <inkview.h>
#include <sys/stat.h>
static App *active_app;
void runtime_attach(App *app) { active_app = app; }
static void browse_timer(void) { browser_poll(active_app); }
static void job_timer(void) { jobs_poll(active_app); }
static void keyboard_timer(void) { keyboard_finish(active_app); }
static void search_timer(void) { keyboard_search_ready(active_app); }
static void repair_timer(void) { repair_library(active_app); }
static void (*const timers[])(void) = {browse_timer, job_timer, keyboard_timer,
                                       search_timer, repair_timer};
void runtime_schedule(App *app, int timer, int milliseconds) {
  runtime_attach(app);
  const char *names[] = {"pocketshelf-browse", "pocketshelf",
                         "pocketshelf-keyboard", "pocketshelf-keyboard",
                         "library-repair"};
  SetHardTimer(names[timer], timers[timer], milliseconds);
}
void runtime_clear(int timer) { ClearTimer(timers[timer]); }
static void keyboard_callback(char *value) {
  keyboard_complete(active_app, value);
}
void runtime_keyboard(App *app, const char *title, char *buffer, int size,
                      int flags) {
  runtime_attach(app);
  OpenKeyboard(title, buffer, size, flags, keyboard_callback);
}
static int event_callback(int event, int p1, int p2) {
  return app_event(active_app, event, p1, p2);
}
int runtime_run(App *app) {
  runtime_attach(app);
  umask(0077);
  curl_global_init(CURL_GLOBAL_DEFAULT);
  InitInkview(TASK_MAKEACTIVE | TASK_FB_RGB24);
  InkViewMain(event_callback);
  curl_global_cleanup();
  return 0;
}
