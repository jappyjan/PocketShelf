#include "app/app.h"
#include "app/state.h"
#include "platform/runtime.h"
int main(void) {
  static App app;
  app_init(&app);
  return runtime_run(&app);
}
