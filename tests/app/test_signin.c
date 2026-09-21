#include "support/app_test.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
void test_signin(App *app) {

  mock_ui.ui_thread = pthread_self();
  app->screen = HOME;
  app_render(app);
  assert(app->screen == SIGNIN &&
         "Logged-out startup must show only the login form");
  assert(!strstr(mock_ui.drawn, "Search the library"));
  assert(!strstr(mock_ui.drawn, "Browse popular books"));
  assert(!strstr(mock_ui.drawn, "Close"));
  assert(app->screen == SIGNIN && mock_ui.keyboard_calls == 0);
  assert(strstr(mock_ui.drawn, "Email") && strstr(mock_ui.drawn, "Password"));
  app_action(app, ACTION_LOGIN);
  assert(!app->foreground.active && strstr(app->notice, "both"));
  app_action(app, ACTION_EMAIL);
  assert(mock_ui.keyboard_calls == 1);
  test_complete_keyboard(app, "reader@example.org");
  assert(!strcmp(app->email, "reader@example.org"));
  assert(mock_ui.keyboard_calls == 1);
  app_action(app, ACTION_PASSWORD);
  assert(mock_ui.keyboard_calls == 2);
  assert(mock_ui.keyboard_flags & KBD_PASSWORD);
  test_complete_keyboard(app, "secret-test-password");
  assert(!app->foreground.active &&
         "Entering a password must not submit the form");
  assert(strstr(mock_ui.drawn, "secret-test-password") == NULL);
  assert(strstr(mock_ui.drawn, "reader@example.org") != NULL);
  app_action(app, ACTION_PASSWORD);
  test_complete_keyboard(app, NULL);
  assert(!app->foreground.active);
  app_action(app, ACTION_LOGIN);
  assert(app->password[0] &&
         "Submitting sign-in must retain the form password until success");
  for (int i = 0; app->foreground.active && i < 200; ++i) {
    struct timespec delay = {0, 1000000};
    nanosleep(&delay, NULL);
    jobs_poll(app);
  }
  assert(!app->foreground.active);
  assert(app->password[0] && "A rejected login must not erase the password");
  assert(strstr(mock_ui.drawn, "Sign-in failed") &&
         "A login failure must have a prominent result screen");
  assert(strstr(mock_ui.drawn, "browser check"));
  assert(mock_ui.error_heading_color == 0x00D00000 &&
         "Errors must be red on the color screen");
  app_action(app, ACTION_BACK); /* back to sign-in */
  assert(app->screen == SIGNIN && app->password[0]);
  fixture.success = 1;
  app_action(app, ACTION_LOGIN);
  for (int i = 0; app->foreground.active && i < 200; ++i) {
    struct timespec delay = {0, 1000000};
    nanosleep(&delay, NULL);
    jobs_poll(app);
  }
  assert(!app->foreground.active && app->account.key[0]);
  assert(!app->password[0] && "Successful sign-in must clear the password");
  assert(app->screen == HOME);
  assert(strstr(mock_ui.drawn, "Search the library"));
  assert(strstr(mock_ui.drawn, "Browse popular books"));
  assert(strstr(mock_ui.drawn, "reader@example.org"));
  assert(mock_ui.header_identity && "Current user belongs at the top right");
  assert(!strstr(mock_ui.drawn, "Close"));
  assert(!app->password[0]);
  const int inner_screens[] = {SETTINGS, LIST, LOCAL, DETAIL};
  for (unsigned i = 0; i < sizeof(inner_screens) / sizeof(inner_screens[0]);
       i++) {
    app->screen = inner_screens[i];
    app_render(app);
    assert(mock_ui.header_identity);
    test_tap_back(app);
    assert(app->screen == (inner_screens[i] == DETAIL ? LIST : HOME));
    assert(mock_ui.close_calls == 0);
  }
  app->screen = HOME;
  app_render(app);
  for (int i = 0; i < app->ui.button_count; i++)
    assert(app->ui.buttons[i].action != ACTION_BACK);
  app_action(app, ACTION_SETTINGS);
  app_action(app, ACTION_SIGNOUT);
  assert(app->screen == SIGNIN);
  assert(!strstr(mock_ui.drawn, "Search the library"));
  app_action(app, ACTION_POPULAR);
  assert(app->screen == SIGNIN && !app->foreground.active);
  app_action(app, ACTION_SEARCH);
  assert(app->screen == SIGNIN && !app->keyboard_active);
  app_event(app, EVT_KEYPRESS, IV_KEY_BACK, 0);
  assert(mock_ui.close_calls == 1 && "Physical Back closes the login screen");
  runtime_run(app);
  assert(mock_ui.initialization_flags & TASK_FB_RGB24);
}
