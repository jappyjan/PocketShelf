#include "core/book.h"
#include "core/text.h"
#include "library/internal.h"
#include "net/http.h"
#include "storage/book_file.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
int library_cover(const Account *a, Book *b, const char *dir, Transfer *t) {
  if (!valid_identifier(b->id) || !valid_identifier(b->hash) ||
      strncmp(b->cover_url, "https://", 8))
    return 0;
  char path[PATH_CAP], tmp[PATH_CAP], err[512];
  if (snprintf(path, sizeof(path), "%s/%s-%s.img", dir, b->id, b->hash) >=
      (int)sizeof(path))
    return 0;
  struct stat st;
  if (stat(path, &st) == 0 && st.st_size > 0) {
    text_copy(b->cover_path, sizeof(b->cover_path), path);
    return 1;
  }
  if (snprintf(tmp, sizeof(tmp), "%s/.cover-XXXXXX", dir) >= (int)sizeof(tmp))
    return 0;
  int fd = mkstemp(tmp);
  if (fd < 0)
    return 0;
  FILE *f = fdopen(fd, "w+b");
  if (!f) {
    close(fd);
    unlink(tmp);
    return 0;
  }
  Sink sink = {.file = f, .transfer = t, .limit = 4 * 1024 * 1024};
  int ok = http_request(a, b->cover_url, NULL, &sink, 2, err, sizeof(err));
  unsigned char h[8] = {0};
  if (fflush(f) != 0)
    ok = 0;
  rewind(f);
  size_t n = fread(h, 1, 8, f);
  ok = ok && n == 8 &&
       ((!memcmp(h, "\x89PNG\r\n\x1a\n", 8)) ||
        (h[0] == 255 && h[1] == 216 && h[2] == 255));
  if (fclose(f) != 0)
    ok = 0;
  if (ok && rename(tmp, path) == 0) {
    text_copy(b->cover_path, sizeof(b->cover_path), path);
    return 1;
  }
  unlink(tmp);
  return 0;
}

int library_download(const Account *a, const Book *b, const char *dir,
                     char *path, size_t path_cap, Transfer *t, char *error,
                     size_t cap) {
  if (!valid_identifier(b->id) || !valid_identifier(b->hash) ||
      !supported_format(b->format)) {
    text_copy(error, cap,
              "This file type is not supported by the PocketBook reader.");
    return 0;
  }
  char endpoint[256];
  snprintf(endpoint, sizeof(endpoint), "/eapi/book/%s/%s/file", b->id, b->hash);
  cJSON *j = library_api(a, endpoint, NULL, t, error, cap);
  if (!j)
    return 0;
  const cJSON *file = json_field(j, "file");
  const char *link = json_string(file, "downloadLink");
  char url[URL_CAP];
  if (!*link) {
    text_copy(
        error, cap,
        "No download is available. Check your account quota on the website.");
    cJSON_Delete(j);
    return 0;
  }
  int len = link[0] == '/' && link[1] != '/'
                ? snprintf(url, sizeof(url), "%s%s", a->base, link)
                : snprintf(url, sizeof(url), "%s", link);
  cJSON_Delete(j);
  if (len < 0 || len >= (int)sizeof(url) || strncmp(url, "https://", 8)) {
    text_copy(error, cap,
              "The server returned an invalid or insecure download URL.");
    return 0;
  }
  char tmp[PATH_CAP];
  if (snprintf(tmp, sizeof(tmp), "%s/.download-XXXXXX", dir) >=
      (int)sizeof(tmp)) {
    text_copy(error, cap, "Download folder path is too long.");
    return 0;
  }
  int fd = mkstemp(tmp);
  if (fd < 0) {
    text_copy(
        error, cap,
        "Cannot create download. Check free space and the download folder.");
    return 0;
  }
  FILE *f = fdopen(fd, "w+b");
  if (!f) {
    close(fd);
    unlink(tmp);
    return 0;
  }
  __sync_lock_test_and_set(&t->percent, 0);
  Sink sink = {.file = f, .transfer = t, .limit = BOOK_LIMIT};
  int ok = http_request(a, url, NULL, &sink, 1, error, cap);
  if (ok && (fflush(f) != 0 || !book_file_valid(f, b->format))) {
    text_copy(error, cap,
              "The download is incomplete or is not the selected book format.");
    ok = 0;
  }
  if (fclose(f) != 0) {
    text_copy(error, cap, "Could not finish writing the book.");
    ok = 0;
  }
  char name[161];
  size_t n = 0;
  for (const unsigned char *s = (const unsigned char *)b->title; *s && n < 160;
       s++)
    name[n++] = (*s < 32 || strchr("/\\:*?\"<>|", *s)) ? '_' : (char)*s;
  while (n && ((unsigned char)name[n - 1] >= 128))
    n--; /* never finish in a partial UTF-8 character */
  name[n] = 0;
  for (size_t i = 0; i < n && (name[i] == '.' || name[i] == ' '); i++)
    name[i] = '_';
  if (!n)
    text_copy(name, sizeof(name), "Book");
  if (ok && snprintf(path, path_cap, "%s/%s-%s-%s.%s", dir, name, b->id,
                     tmp + strlen(tmp) - 6, b->format) >= (int)path_cap) {
    text_copy(error, cap, "Book filename is too long.");
    ok = 0;
  }
  if (ok) {
    int reserve = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (reserve < 0) {
      text_copy(error, cap, "Cannot reserve the book filename.");
      ok = 0;
    } else {
      close(reserve);
      if (rename(tmp, path) != 0) {
        unlink(path);
        text_copy(error, cap, "Cannot save the completed book.");
        ok = 0;
      }
    }
  }
  if (!ok)
    unlink(tmp);
  return ok;
}
