#include "storage/account.h"
#include "storage/downloads.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void create_file(const char *directory, const char *name) {
  char path[PATH_CAP];
  snprintf(path, sizeof(path), "%s/%s", directory, name);
  FILE *file = fopen(path, "w");
  assert(file);
  assert(fputs("fixture", file) >= 0);
  assert(fclose(file) == 0);
}
int main(void) {
  char directory[] = "/tmp/pocketshelf-storage-XXXXXX";
  assert(mkdtemp(directory));
  char config[PATH_CAP], temporary[PATH_CAP];
  snprintf(config, sizeof(config), "%s/account.json", directory);
  snprintf(temporary, sizeof(temporary), "%s/account.json.tmp", directory);
  Account saved = {.base = "https://library.example.org",
                   .user = "123",
                   .key = "fixture-session",
                   .email = "reader@example.org"};
  assert(account_save(config, &saved));
  Account loaded = {0};
  account_load(config, &loaded);
  assert(!memcmp(&saved, &loaded, sizeof(saved)));
  struct stat info;
  assert(stat(config, &info) == 0 && (info.st_mode & 0777) == 0600);
  assert(access(temporary, F_OK) != 0);
  account_forget(config);
  assert(access(config, F_OK) != 0);

  const char *names[] = {"Zebra.epub", "Alpha.fb2.zip", "notes.txt",
                         "ignore.part", ".hidden.epub"};
  for (size_t i = 0; i < sizeof(names) / sizeof(*names); i++)
    create_file(directory, names[i]);
  char subdirectory[PATH_CAP];
  snprintf(subdirectory, sizeof(subdirectory), "%s/folder.epub", directory);
  assert(mkdir(subdirectory, 0700) == 0);
  LocalBooks books;
  downloads_scan(&books, directory);
  assert(books.count == 3);
  assert(!strcmp(books.names[0], "Alpha.fb2.zip"));
  assert(!strcmp(books.names[1], "Zebra.epub"));
  assert(!strcmp(books.names[2], "notes.txt"));
  for (size_t i = 0; i < sizeof(names) / sizeof(*names); i++) {
    char path[PATH_CAP];
    snprintf(path, sizeof(path), "%s/%s", directory, names[i]);
    assert(unlink(path) == 0);
  }
  assert(rmdir(subdirectory) == 0);
  assert(rmdir(directory) == 0);
  puts("Account persistence and downloaded-book discovery passed.");
  return 0;
}
