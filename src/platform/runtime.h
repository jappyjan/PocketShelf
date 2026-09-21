#ifndef POCKETSHELF_SRC_PLATFORM_RUNTIME_H
#define POCKETSHELF_SRC_PLATFORM_RUNTIME_H
#include "app/app.h"
enum { TIMER_BROWSE, TIMER_JOB, TIMER_KEYBOARD, TIMER_SEARCH, TIMER_REPAIR };
/* InkView owns one active app. This adapter translates its context-free
 * callbacks. */
int runtime_run(App *);
void runtime_attach(App *);
void runtime_schedule(App *, int timer, int milliseconds);
void runtime_clear(int timer);
void runtime_keyboard(App *, const char *, char *, int, int);

#endif
