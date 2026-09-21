#include "ui/widgets.h"
#include <stdlib.h>
int sx(int x) { return x * ScreenWidth() / 600; }

int sy(int y) { return y * ScreenHeight() / 800; }

void text_color(int x, int y, int w, int h, const char *s, ifont *f,
                int color) {
  SetFont(f, color);
  DrawTextRect(sx(x), sy(y), sx(w), sy(h), s, ALIGN_LEFT | VALIGN_TOP);
}

void text_at(int x, int y, int w, int h, const char *s, ifont *f) {
  text_color(x, y, w, h, s, f, BLACK);
}

void button(Ui *ui, int x, int y, int w, int h, const char *label, int action) {
  DrawRect(sx(x), sy(y), sx(w), sy(h), BLACK);
  text_at(x + 10, y + 10, w - 20, h - 14, label, ui->font);
  if (ui->button_count < 32)
    ui->buttons[ui->button_count++] = (Button){x, y, w, h, action};
}

void arrow_button(Ui *ui, int x, int y, int action, int right) {
  int a = right ? x + 34 : x + 18, b = right ? x + 18 : x + 34;
  for (int d = 0; d < 2; d++) {
    DrawLine(sx(a), sy(y + 26 + d), sx(b), sy(y + 12 + d), BLACK);
    DrawLine(sx(a), sy(y + 26 + d), sx(b), sy(y + 40 + d), BLACK);
    DrawLine(sx(x + 18), sy(y + 26 + d), sx(x + 42), sy(y + 26 + d), BLACK);
  }
  ui->buttons[ui->button_count++] = (Button){x, y, 60, 52, action};
}

void draw_cover(Ui *ui, const Book *b, int x, int y, int w, int h) {
  ibitmap *image =
      b->cover_path[0] ? LoadImageToFormat(b->cover_path, kFmtRGB24) : NULL;
  if (image && image->width > 0 && image->height > 0) {
    int width = sx(w), height = sy(h);
    if ((long long)image->width * height > (long long)image->height * width)
      height = (long long)image->height * width / image->width;
    else
      width = (long long)image->width * height / image->height;
    DrawBitmapRect(sx(x) + (sx(w) - width) / 2, sy(y) + (sy(h) - height) / 2,
                   width, height, image, STRETCH);
  } else {
    DrawRect(sx(x), sy(y), sx(w), sy(h), 0x888888);
    text_at(x + 4, y + h / 3, w - 8, h / 2, "No cover", ui->small);
  }
  free(image);
}
