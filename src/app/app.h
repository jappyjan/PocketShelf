#ifndef POCKETSHELF_SRC_APP_APP_H
#define POCKETSHELF_SRC_APP_APP_H
typedef struct App App;
void app_init(App *);
int app_event(App *, int event, int p1, int p2);
void app_action(App *, int action);
void app_render(App *);
void app_scroll(App *, int offset);

#endif
