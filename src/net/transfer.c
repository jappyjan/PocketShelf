#include "net/transfer.h"
#include <stdlib.h>
#include <string.h>
void transfer_cancel(Transfer *t) { __sync_lock_test_and_set(&t->cancel, 1); }

int transfer_percent(Transfer *t) {
  return __sync_fetch_and_add(&t->percent, 0);
}

int transfer_progress(void *ctx, double total, double now, double ut,
                      double un) {
  (void)ut;
  (void)un;
  Transfer *t = ctx;
  if (total > 0)
    __sync_lock_test_and_set(&t->percent, (int)(now * 100 / total));
  return __sync_fetch_and_add(&t->cancel, 0) != 0;
}

size_t sink_write(void *ptr, size_t size, size_t nmemb, void *ctx) {
  Sink *s = ctx;
  size_t n = size * nmemb;
  if (n > s->limit - s->length)
    return 0;
  if (s->file) {
    size_t written = fwrite(ptr, 1, n, s->file);
    s->length += written;
    return written;
  }
  char *p = realloc(s->data, s->length + n + 1);
  if (!p)
    return 0;
  s->data = p;
  memcpy(p + s->length, ptr, n);
  s->length += n;
  p[s->length] = 0;
  return n;
}
