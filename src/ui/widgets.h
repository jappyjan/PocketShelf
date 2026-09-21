#ifndef POCKETSHELF_SRC_UI_WIDGETS_H
#define POCKETSHELF_SRC_UI_WIDGETS_H
#include "core/models.h"
#include <inkview.h>
#define COLOR_ERROR 0x00D00000
typedef struct {
  int x, y, w, h, action;
} Button;
typedef struct {
  ifont *font, *small, *large;
  Button buttons[32];
  int button_count;
} Ui;
int sx(int);
int sy(int);
void text_color(int, int, int, int, const char *, ifont *, int);
void text_at(int, int, int, int, const char *, ifont *);
void button(Ui *, int, int, int, int, const char *, int);
void arrow_button(Ui *, int, int, int, int);
void draw_cover(Ui *, const Book *, int, int, int, int);

#endif
