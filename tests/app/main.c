#include "support/app_test.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
int main(void) {
  static App app;
  app_init(&app);
  test_signin(&app);
  test_books(&app);
  test_browse(&app);
  app_event(&app, EVT_EXIT, 0, 0);
  return 0;
}
