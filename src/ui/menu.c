#include "app/state.h"
#include "ui/screens.h"
#include <stdio.h>
void screen_home(App *app) {

  text_at(24, 110, 552, 70, "Find your next book.", app->ui.large);
  text_at(24, 178, 552, 68,
          "Download books and open them with PocketBook's built-in reader.",
          app->ui.font);
  button(&app->ui, 24, 276, 552, 66, "Search the library", ACTION_SEARCH);
  button(&app->ui, 24, 362, 552, 66, "Browse popular books", ACTION_POPULAR);
  button(&app->ui, 24, 448, 552, 66, "Downloaded books", ACTION_DOWNLOADS);
  button(&app->ui, 24, 534, 552, 66, "Account & settings", ACTION_SETTINGS);
  text_at(24, 624, 552, 36, "Account connected", app->ui.small);
}

void screen_settings(App *app) {

  text_at(24, 105, 552, 38, "Account & settings", app->ui.large);
  text_at(24, 163, 552, 52, app->account.base, app->ui.small);
  button(&app->ui, 24, 220, 552, 55, "Change library address", ACTION_ADDRESS);
  text_at(24, 292, 552, 55,
          app->account.email[0] ? app->account.email : app->account.user,
          app->ui.small);
  char filter[100];
  snprintf(filter, sizeof(filter), "Search format: %s (tap to change)",
           *app->format ? app->format : "All formats");
  button(&app->ui, 24, 364, 552, 55, filter, ACTION_FORMAT);
  button(&app->ui, 24, 436, 552, 55, "Sign out & forget session",
         ACTION_SIGNOUT);
  button(&app->ui, 24, 508, 552, 55, "Refresh downloads in Library",
         ACTION_REPAIR);
  text_at(24, 584, 552, 116,
          "Your password is used only to sign in. A session token is saved "
          "on this device. Changing the address signs you out.",
          app->ui.small);
}
