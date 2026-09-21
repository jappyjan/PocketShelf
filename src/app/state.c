#include "app/state.h"
#include "app/app.h"
#include "platform/runtime.h"
#include <string.h>
void app_init(App *app) {
  memset(app, 0, sizeof(*app));
  strcpy(app->account.base, "https://z-library.sk");
  app->screen = SIGNIN;
  app->browser.page = 1;
  app->foreground.last_percent = -1;
  pthread_mutex_init(&app->lock, NULL);
  runtime_attach(app);
}
