#ifndef POCKETSHELF_SRC_STORAGE_DOWNLOADS_H
#define POCKETSHELF_SRC_STORAGE_DOWNLOADS_H
#define MAX_LOCAL 256
typedef struct {
  int count;
  char names[MAX_LOCAL][256];
} LocalBooks;
void downloads_scan(LocalBooks *, const char *directory);

#endif
