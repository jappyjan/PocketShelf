#ifndef POCKETSHELF_SRC_NET_TRANSFER_H
#define POCKETSHELF_SRC_NET_TRANSFER_H
#include "core/models.h"
#include <stdio.h>
#define JSON_LIMIT (4 * 1024 * 1024)
#define BOOK_LIMIT ((size_t)-1)
typedef struct {
  char *data;
  size_t length;
  FILE *file;
  Transfer *transfer;
  size_t limit;
} Sink;
void transfer_cancel(Transfer *);
int transfer_percent(Transfer *);
int transfer_progress(void *, double, double, double, double);
size_t sink_write(void *, size_t, size_t, void *);

#endif
