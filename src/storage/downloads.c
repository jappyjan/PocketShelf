#include "storage/downloads.h"
#include "core/book.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
static int compare_names(const void *a, const void *b) { return strcmp(a, b); }

void downloads_scan(LocalBooks *books, const char *directory) {
  books->count = 0;
  DIR *dir = opendir(directory);
  if (!dir)
    return;
  struct dirent *e;
  while ((e = readdir(dir)) && books->count < MAX_LOCAL) {
    const char *ext = strrchr(e->d_name, '.');
    if (ext && !strcmp(ext, ".zip")) {
      const char *p = ext;
      while (p > e->d_name && p[-1] != '.')
        p--;
      if (p > e->d_name)
        ext = p - 1;
    }
    if (e->d_name[0] == '.' || !ext || !supported_format(ext + 1))
      continue;
    char path[PATH_CAP];
    struct stat st;
    snprintf(path, sizeof(path), "%s/%s", directory, e->d_name);
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode))
      continue;
    snprintf(books->names[books->count++], 256, "%s", e->d_name);
  }
  closedir(dir);
  qsort(books->names, books->count, sizeof(books->names[0]), compare_names);
}
