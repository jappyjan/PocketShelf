#ifndef POCKETSHELF_SRC_APP_JOBS_H
#define POCKETSHELF_SRC_APP_JOBS_H
#include "app/app.h"
void jobs_poll(App *);
void jobs_launch(App *);
void jobs_start(App *, int type);
void jobs_shutdown(App *);

#endif
