#include "app/keyboard.h"
#include "app/browser.h"
#include "app/state.h"
#include "core/text.h"
#include "platform/runtime.h"
#include "storage/account.h"
#include "storage/paths.h"
#include <stdio.h>
#include <string.h>
void keyboard_finish(App *app) {
  app->keyboard_active = 0;
  app_render(app);
}

void keyboard_search_ready(App *app) {
  app->keyboard_active = 0;
  if (app->query[0])
    browser_begin(app, 0);
  else
    app_render(app);
}

void keyboard_complete(App *app, char *value) {
  /* InkView still owns the modal during this callback. Only update state;
     redraw or start another operation after it has returned and closed. */
  if (value) {
    if (app->edit_kind == EDIT_QUERY) {
      snprintf(app->query, sizeof(app->query), "%s", value);
    } else if (app->edit_kind == EDIT_BASE) {
      size_t n = strlen(value);
      while (n && value[n - 1] == '/')
        value[--n] = 0;
      if (!valid_base(value)) {
        app->notice_is_error = 1;
        snprintf(app->notice, sizeof(app->notice),
                 "Use https:// followed by a hostname, without a path.");
      } else {
        snprintf(app->account.base, sizeof(app->account.base), "%s", value);
        secure_wipe(app->account.user, sizeof(app->account.user));
        secure_wipe(app->account.key, sizeof(app->account.key));
        snprintf(app->notice, sizeof(app->notice), "%s",
                 account_save(CONFIG_FILE, &app->account)
                     ? "Address saved. Sign in to this library."
                     : "Could not save settings.");
      }
    } else if (app->edit_kind == EDIT_EMAIL) {
      snprintf(app->email, sizeof(app->email), "%s", value);
    } else if (app->edit_kind == EDIT_PASSWORD) {
      snprintf(app->password, sizeof(app->password), "%s", value);
    }
  }
  int search = value && app->edit_kind == EDIT_QUERY;
  secure_wipe(app->edit, sizeof(app->edit));
  runtime_schedule(app, search ? TIMER_SEARCH : TIMER_KEYBOARD, 100);
}

void keyboard_edit(App *app, int kind, const char *title, const char *value,
                   int flags) {
  app->edit_kind = kind;
  snprintf(app->edit, sizeof(app->edit), "%s", value);
  app->keyboard_active = 1;
  runtime_keyboard(app, title, app->edit, kind == EDIT_QUERY ? 511 : 255,
                   flags);
}
