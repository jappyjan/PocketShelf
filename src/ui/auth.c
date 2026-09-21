#include "app/state.h"
#include "ui/screens.h"
#include <stdio.h>
void screen_signin(App *app) {

  text_at(24, 108, 552, 48, "Sign in to Z-Library", app->ui.large);
  text_at(24, 170, 552, 26, "Library address", app->ui.small);
  button(&app->ui, 24, 202, 552, 54, app->account.base, ACTION_ADDRESS);
  text_at(24, 282, 552, 30, "Email", app->ui.small);
  button(&app->ui, 24, 314, 552, 64,
         app->email[0] ? app->email : "Enter your email", ACTION_EMAIL);
  text_at(24, 402, 552, 30, "Password", app->ui.small);
  button(&app->ui, 24, 434, 552, 64,
         app->password[0] ? "********" : "Enter your password", 25);
  button(&app->ui, 24, 548, 552, 66, "Sign in", ACTION_LOGIN);
  text_at(24, 646, 552, 54,
          "Tap a field to edit it. Your password is not saved.", app->ui.small);
}

void screen_signin_error(App *app) {

  FillArea(sx(24), sy(108), sx(552), sy(5), COLOR_ERROR);
  DrawRect(sx(24), sy(108), sx(552), sy(416), COLOR_ERROR);
  text_color(40, 130, 520, 64, "Sign-in failed", app->ui.large, COLOR_ERROR);
  text_color(40, 223, 520, 280, app->notice, app->ui.font, COLOR_ERROR);
  text_at(24, 646, 552, 54,
          "Your entries are kept so you can correct them or retry.",
          app->ui.small);
}
