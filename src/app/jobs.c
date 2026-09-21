#include "app/jobs.h"
#include "app/state.h"
#include "core/text.h"
#include "library/client.h"
#include "platform/library.h"
#include "platform/runtime.h"
#include "storage/account.h"
#include "storage/paths.h"
#include <stdio.h>
#include <string.h>
static void *run_job(void *context) {
  App *app = context;
  if (app->foreground.job.type == LOGIN)
    app->foreground.job.ok = library_login(
        &app->foreground.job.account, app->foreground.job.email,
        app->foreground.job.password, &app->foreground.job.transfer,
        app->foreground.job.error, sizeof(app->foreground.job.error));
  else if (app->foreground.job.type == DETAILS) {
    app->foreground.job.ok = library_details(
        &app->foreground.job.account, &app->foreground.job.book,
        &app->foreground.job.transfer, app->foreground.job.error,
        sizeof(app->foreground.job.error));
    if (app->foreground.job.ok)
      library_cover(&app->foreground.job.account, &app->foreground.job.book,
                    COVER_DIR, &app->foreground.job.transfer);
  } else
    app->foreground.job.ok = library_download(
        &app->foreground.job.account, &app->foreground.job.book, BOOK_DIR,
        app->foreground.job.path, sizeof(app->foreground.job.path),
        &app->foreground.job.transfer, app->foreground.job.error,
        sizeof(app->foreground.job.error));
  secure_wipe(app->foreground.job.password,
              sizeof(app->foreground.job.password));
  fprintf(stderr, "[pocketshelf] job_finished type=%d success=%d\n",
          app->foreground.job.type, app->foreground.job.ok);
  fflush(stderr);
  pthread_mutex_lock(&app->lock);
  app->foreground.job.done = 1;
  pthread_mutex_unlock(&app->lock);
  return NULL;
}

void jobs_poll(App *app) {
  pthread_mutex_lock(&app->lock);
  int done = app->foreground.job.done;
  pthread_mutex_unlock(&app->lock);
  if (!done) {
    int p = transfer_percent(&app->foreground.job.transfer);
    if (app->foreground.job.type == DOWNLOAD &&
        (p == 100 || p / 10 != app->foreground.last_percent / 10) &&
        p != app->foreground.last_percent) {
      app->foreground.last_percent = p;
      app_render(app);
    }
    runtime_schedule(app, TIMER_JOB, 500);
    return;
  }
  pthread_join(app->foreground.thread, NULL);
  app->foreground.active = 0;

  app->notice_is_error = !app->foreground.job.ok;
  if (!app->foreground.job.ok) {
    snprintf(app->notice, sizeof(app->notice), "%s",
             app->foreground.job.error[0] ? app->foreground.job.error
                                          : "Operation failed.");
    if (app->foreground.job.type == LOGIN)
      app->screen = SIGNIN_ERROR;
  } else if (app->foreground.job.type == LOGIN) {
    secure_wipe(app->password, sizeof(app->password));
    app->account = app->foreground.job.account;
    snprintf(app->account.email, sizeof(app->account.email), "%s",
             app->foreground.job.email);
    app->screen = HOME;
    snprintf(
        app->notice, sizeof(app->notice), "%s",
        account_save(CONFIG_FILE, &app->account)
            ? "Signed in. Ready to find books."
            : "Signed in for this session; could not save account settings.");
  } else if (app->foreground.job.type == DETAILS) {
    app->selected = app->foreground.job.book;
    app->screen = DETAIL;
    app->notice[0] = 0;
  } else {
    snprintf(app->opened, sizeof(app->opened), "%s", app->foreground.job.path);
    int registered = register_book(app->foreground.job.path);
    app->notice_is_error = !registered;
    snprintf(app->notice, sizeof(app->notice), "%s",
             registered
                 ? "Book saved. Library update requested. Tap Open to read."
                 : "Book saved, but could not hand it to the Library. Try "
                   "Refresh downloads in settings.");
    app->screen = DETAIL;
  }
  app_render(app);
}

void jobs_launch(App *app) {
  app->foreground.active = 1;
  app->foreground.last_percent = -1;
  app->notice[0] = 0;
  app_render(app);
  fprintf(stderr, "[pocketshelf] job_started type=%d\n",
          app->foreground.job.type);
  int connection = NetConnect2(NULL, 1);
  fprintf(stderr, "[pocketshelf] connection_result=%d\n", connection);
  fflush(stderr);
  if (pthread_create(&app->foreground.thread, NULL, run_job, app) != 0) {
    app->foreground.active = 0;
    secure_wipe(app->foreground.job.password,
                sizeof(app->foreground.job.password));
    app->notice_is_error = 1;
    snprintf(app->notice, sizeof(app->notice),
             "Could not start the network task.");
    if (app->foreground.job.type == LOGIN)
      app->screen = SIGNIN_ERROR;
    fprintf(stderr, "[pocketshelf] worker_start_failed\n");
    fflush(stderr);
    app_render(app);
    return;
  }
  runtime_schedule(app, TIMER_JOB, 500);
}

void jobs_start(App *app, int type) {
  if (app->foreground.active)
    return;
  if (!signed_in(app) && type != LOGIN) {
    app->screen = SIGNIN;
    snprintf(app->notice, sizeof(app->notice),
             "Sign in before browsing or downloading.");
    app_render(app);
    return;
  }
  memset(&app->foreground.job, 0, sizeof(app->foreground.job));
  app->foreground.job.type = type;
  app->foreground.job.account = app->account;
  app->foreground.job.book = app->selected;
  jobs_launch(app);
}

void jobs_shutdown(App *app) {
  runtime_clear(TIMER_JOB);
  if (app->foreground.active) {
    transfer_cancel(&app->foreground.job.transfer);
    pthread_join(app->foreground.thread, NULL);
  }
  app->foreground.active = 0;
  secure_wipe(&app->foreground.job, sizeof(app->foreground.job));
}
