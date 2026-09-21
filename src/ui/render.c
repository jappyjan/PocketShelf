#include "app/state.h"
#include "net/transfer.h"
#include "ui/catalog.h"
#include "ui/screens.h"
#include <stdio.h>
void app_render(App *app) {
  if (!signed_in(app) && app->screen != SIGNIN && app->screen != SIGNIN_ERROR)
    app->screen = SIGNIN;
  ClearScreen();
  app->ui.button_count = 0;
  if (!app->foreground.active &&
      (app->screen == SETTINGS || app->screen == LIST ||
       app->screen == DETAIL || app->screen == LOCAL ||
       app->screen == SIGNIN_ERROR))
    arrow_button(&app->ui, 24, 18, ACTION_BACK, 0);
  else
    text_at(24, 20, 240, 48, "PocketShelf", app->ui.large);
  if (signed_in(app)) {
    char label[280];
    if (app->account.email[0])
      snprintf(label, sizeof(label), "%s", app->account.email);
    else
      snprintf(label, sizeof(label), "User %s", app->account.user);
    SetFont(app->ui.small, BLACK);
    DrawTextRect(sx(280), sy(20), sx(296), sy(52), label,
                 ALIGN_RIGHT | VALIGN_TOP);
  }
  DrawLine(sx(24), sy(82), sx(576), sy(82), BLACK);
  if (app->foreground.active) {
    text_at(24, 135, 550, 100,
            app->foreground.job.type == LOGIN      ? "Signing in..."
            : app->foreground.job.type == DOWNLOAD ? "Downloading your book..."
                                                   : "Loading books...",
            app->ui.large);
    text_at(24, 250, 550, 150,
            "Keep Wi-Fi connected. You can cancel without leaving an "
            "incomplete book in your library.",
            app->ui.font);
    char p[64];
    int percent = transfer_percent(&app->foreground.job.transfer);
    snprintf(p, sizeof(p), percent > 0 ? "Progress: %d%%" : "Connecting...",
             percent);
    text_at(24, 410, 550, 40, p, app->ui.font);
    button(&app->ui, 24, 495, 552, 60, "Cancel", ACTION_CANCEL);
  } else if (app->screen == HOME) {
    screen_home(app);
  } else if (app->screen == SETTINGS) {
    screen_settings(app);
  } else if (app->screen == SIGNIN_ERROR) {
    screen_signin_error(app);
  } else if (app->screen == SIGNIN) {
    screen_signin(app);
  } else if (app->screen == LIST) {
    draw_scroll_list(app, app->browser.popular ? "Popular books" : app->query,
                     app->browser.active ? "Finding books..."
                     : app->browser.failed
                         ? "Could not load books."
                         : "No books found. Try another search or format.");
  } else if (app->screen == DETAIL) {
    screen_detail(app);
  } else if (app->screen == LOCAL) {
    draw_scroll_list(app, "Downloaded books",
                     "Your downloaded books will appear here.");
  }
  if (app->notice[0] && !app->foreground.active &&
      app->screen != SIGNIN_ERROR && !is_list_screen(app))
    text_color(24, 718, 552, 78, app->notice, app->ui.small,
               app->notice_is_error ? COLOR_ERROR : BLACK);
  if (app->scroll_update)
    PartialUpdate(0, sy(96), ScreenWidth(), ScreenHeight() - sy(96));
  else
    FullUpdate();
}
