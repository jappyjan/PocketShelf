#include "support/app_test.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
void test_drain_browse(App *app) {
  for (int i = 0; app->browser.active && i < 1000; i++) {
    struct timespec delay = {0, 1000000};
    nanosleep(&delay, NULL);
    browser_poll(app);
  }
  assert(!app->browser.active);
}

void test_complete_keyboard(App *app, const char *value) {
  if (value)
    snprintf(mock_ui.keyboard_buffer, 512, "%s", value);
  mock_ui.in_keyboard_callback = 1;
  mock_ui.callback(value ? mock_ui.keyboard_buffer : NULL);
  mock_ui.in_keyboard_callback = 0;
  int previous_screen = app->screen;
  app_event(app, EVT_POINTERUP, sx(500), sy(35));
  assert(app->screen == previous_screen &&
         "Ignore pointer-up events left over from the closing keyboard");
  assert(mock_ui.nested_keyboard == 0 &&
         "Opening the password keyboard from the email keyboard callback "
         "causes modal teardown to close it");
  if (mock_ui.pending_timer) {
    void (*cb)(void) = mock_ui.pending_timer;
    mock_ui.pending_timer = NULL;
    cb();
  }
}

void test_tap_back(App *app) {
  int found = 0;
  Button target = {0};
  for (int i = 0; i < app->ui.button_count; i++) {
    if (app->ui.buttons[i].action == ACTION_BACK) {
      target = app->ui.buttons[i];
      found = 1;
      break;
    }
  }
  assert(found && "Every inner menu page must have a visible Back button");
  app_event(app, EVT_POINTERUP, sx(target.x + target.w / 2),
            sy(target.y + target.h / 2));
}
