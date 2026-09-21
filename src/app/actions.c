#include "app/browser.h"
#include "app/jobs.h"
#include "app/keyboard.h"
#include "app/state.h"
#include "core/book.h"
#include "core/text.h"
#include "net/transfer.h"
#include "platform/library.h"
#include "storage/account.h"
#include "storage/paths.h"
#include "ui/catalog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void app_action(App *app, int action) {
  if (app->keyboard_active)
    return;
  if (app->foreground.active) {
    if (action == ACTION_BACK || action == ACTION_CANCEL) {
      transfer_cancel(&app->foreground.job.transfer);
    }
    return;
  }
  if (!signed_in(app) && action != ACTION_BACK && action != ACTION_ADDRESS &&
      action != ACTION_EMAIL && action != ACTION_PASSWORD &&
      action != ACTION_LOGIN) {
    app->screen = SIGNIN;
    app_render(app);
    return;
  }
  app->notice[0] = 0;
  app->notice_is_error = 0;
  if (action == ACTION_BACK) {
    if (app->screen == HOME || app->screen == SIGNIN) {
      secure_wipe(app->password, sizeof(app->password));
      CloseApp();
      return;
    }
    if (app->screen == SIGNIN_ERROR) {
      app->screen = SIGNIN;
    } else
      app->screen = app->screen == DETAIL ? LIST : HOME;
  } else if (action == ACTION_SEARCH) {
    keyboard_edit(app, EDIT_QUERY, "Title, author or ISBN", app->query, 0);
    return;
  } else if (action == ACTION_POPULAR) {
    browser_begin(app, 1);
    return;
  } else if (action == ACTION_DOWNLOADS) {
    downloads_scan(&app->downloads, BOOK_DIR);
    app->local_scroll = 0;
    app->screen = LOCAL;
  } else if (action == ACTION_SETTINGS)
    app->screen = SETTINGS;
  else if (action == ACTION_ADDRESS) {
    keyboard_edit(app, EDIT_BASE, "HTTPS library address", app->account.base,
                  0);
    return;
  } else if (action == ACTION_EMAIL && app->screen == SIGNIN) {
    keyboard_edit(app, EDIT_EMAIL, "Email", app->email, 0);
    return;
  } else if (action == ACTION_PASSWORD && app->screen == SIGNIN) {
    keyboard_edit(app, EDIT_PASSWORD, "Password", app->password, KBD_PASSWORD);
    return;
  } else if (action == ACTION_LOGIN && app->screen == SIGNIN) {
    if (!app->email[0] || !app->password[0]) {
      app->notice_is_error = 1;
      snprintf(app->notice, sizeof(app->notice),
               "Enter both your email and password.");
    } else {
      memset(&app->foreground.job, 0, sizeof(app->foreground.job));
      app->foreground.job.type = LOGIN;
      app->foreground.job.account = app->account;
      snprintf(app->foreground.job.email, sizeof(app->foreground.job.email),
               "%s", app->email);
      snprintf(app->foreground.job.password,
               sizeof(app->foreground.job.password), "%s", app->password);
      jobs_launch(app);
      return;
    }
  } else if (action == ACTION_FORMAT) {
    if (!format_option(++app->format_index))
      app->format_index = 0;
    snprintf(app->format, sizeof(app->format), "%s",
             format_option(app->format_index));
  } else if (action == ACTION_REPAIR) {
    start_library_repair(app);
    snprintf(app->notice, sizeof(app->notice),
             "Updating downloaded books in the PocketBook Library.");
  } else if (action == ACTION_SIGNOUT) {
    browser_cancel(app);
    free(app->browser.books);
    app->browser.books = NULL;
    app->browser.count = app->browser.capacity = 0;
    secure_wipe(app->account.user, sizeof(app->account.user));
    secure_wipe(app->account.key, sizeof(app->account.key));
    secure_wipe(app->account.email, sizeof(app->account.email));
    secure_wipe(app->password, sizeof(app->password));
    app->screen = SIGNIN;
    secure_wipe(&app->foreground.job, sizeof(app->foreground.job));
    account_forget(CONFIG_FILE);
    snprintf(app->notice, sizeof(app->notice),
             "Signed out. Saved session removed.");
  } else if (action == ACTION_RETRY) {
    app->browser.failed = 0;
    browser_prefetch(app);
  } else if (action == ACTION_DOWNLOAD) {
    jobs_start(app, DOWNLOAD);
    return;
  } else if (action == ACTION_DESCRIPTION || action == ACTION_INFO) {
    app->detail.tab = action == ACTION_INFO;
    app->detail.page = 0;
    app->detail.offsets[0] = 0;
  } else if (action == ACTION_TEXT_PREVIOUS && app->detail.page > 0)
    app->detail.page--;
  else if (action == ACTION_TEXT_NEXT && app->detail.more)
    app->detail.page++;
  else if (action == ACTION_OPEN && app->opened[0]) {
    OpenBook(app->opened, "r", 0);
    return;
  } else if (app->screen == LIST && action >= BOOK_ACTION &&
             action < BOOK_ACTION + app->browser.count) {
    app->selected = browser_book(app, action - BOOK_ACTION);
    app->opened[0] = 0;
    app->screen = DETAIL;
    app->detail.tab = app->detail.page = 0;
    app->detail.offsets[0] = 0;
    jobs_start(app, DETAILS);
    return;
  } else if (app->screen == LOCAL && action >= LOCAL_ACTION &&
             action - LOCAL_ACTION < app->downloads.count) {
    char p[PATH_CAP];
    snprintf(p, sizeof(p), "%s/%s", BOOK_DIR,
             app->downloads.names[action - LOCAL_ACTION]);
    OpenBook(p, "r", 0);
    return;
  }
  app_render(app);
  browser_prefetch(app);
}
