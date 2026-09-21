#include "cJSON.h"
#include "core/book.h"
#include "core/text.h"
#include "library/client.h"
#include "library/parser.h"
#include "net/http.h"
#include "storage/book_file.h"
#include <assert.h>
#include <curl/curl.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static int calls, mode;
static char payload[4096];
int http_request(const Account *a, const char *url, const char *post,
                 Sink *sink, int file, char *error, size_t cap) {
  (void)a;
  calls++;
  if (file == 2) {
    assert(!strcmp(url, "https://covers.example.org/cover.png"));
    const unsigned char png[] = {137, 80, 78, 71, 13, 10, 26, 10};
    return sink_write((void *)png, 1, sizeof(png), sink) == sizeof(png);
  }
  if (post)
    snprintf(payload, sizeof(payload), "%s", post);
  const char *body = "";
  if (strstr(url, "/rpc.php"))
    body = "{\"response\":{\"user_id\":123,\"user_key\":\"session\"}}";
  else if (strstr(url, "/most-popular")) {
    /* This endpoint returns one full collection, ignoring page/limit. */
    cJSON *root = cJSON_CreateObject();
    cJSON *books = cJSON_AddArrayToObject(root, "books");
    for (int i = 0; i < 100; i++) {
      cJSON *b = cJSON_CreateObject();
      cJSON_AddNumberToObject(b, "id", i + 1);
      cJSON_AddStringToObject(b, "hash", "abc");
      cJSON_AddItemToArray(books, b);
    }
    char *json = cJSON_PrintUnformatted(root);
    int ok = sink_write(json, 1, strlen(json), sink) == strlen(json);
    free(json);
    cJSON_Delete(root);
    return ok;
  } else if (strstr(url, "/search"))
    body = "{\"books\":[]}";
  else if (!file) {
    assert(strstr(url, "/eapi/book/123/abc/file"));
    body = "{\"success\":1,\"file\":{\"downloadLink\":\"https://"
           "files.example.org/book\"}}";
  } else {
    assert(!strcmp(url, "https://files.example.org/book"));
    if (mode == 2) {
      sink_write("PK\003\004partial", 1, 11, sink);
      text_copy(error, cap, "Connection lost.");
      return 0;
    }
    if (mode == 3) {
      unsigned char mobi[80] = {0};
      memcpy(mobi + 60, "BOOKMOBI", 8);
      return sink_write(mobi, 1, sizeof(mobi), sink) == sizeof(mobi);
    }
    body = mode == 1 ? "<html>Please login</html>" : "PK\003\004testbook";
  }
  return sink_write((void *)body, 1, strlen(body), sink) == strlen(body);
}
static int entries(const char *dir) {
  DIR *d = opendir(dir);
  assert(d);
  int n = 0;
  struct dirent *e;
  while ((e = readdir(d)))
    if (strcmp(e->d_name, ".") && strcmp(e->d_name, ".."))
      n++;
  closedir(d);
  return n;
}
int main(void) {
  curl_global_init(CURL_GLOBAL_DEFAULT);
  Account a = {.base = "https://library.example.org"};
  Transfer t = {0};
  char err[512], path[PATH_CAP], second[PATH_CAP];
  assert(library_login(&a, "a+b@example.org", "p&= /", &t, err, sizeof(err)));
  assert(strstr(payload, "email=a%2Bb%40example.org"));
  assert(strstr(payload, "password=p%26%3D%20%2F"));
  assert(!strcmp(a.key, "session"));
  Results r;
  assert(library_search(&a, "a & b", "epub", 2, 0, &r, &t, err, sizeof(err)));
  assert(strstr(payload, "message=a%20%26%20b"));
  assert(strstr(payload, "page=2"));
  assert(strstr(payload, "extensions%5B0%5D=epub"));
  assert(library_search(&a, "", "", 1, 1, &r, &t, err, sizeof(err)));
  assert(r.count == 100 && !r.has_more &&
         "Popular must retain the entire collection without inventing page 2");
  char dir[] = "/tmp/pocketshelf-test-XXXXXX";
  assert(mkdtemp(dir));
  Book b = {.id = "123",
            .hash = "abc",
            .title = "../../Book: test",
            .format = "epub"};
  assert(
      library_download(&a, &b, dir, path, sizeof(path), &t, err, sizeof(err)));
  assert(!strncmp(path, dir, strlen(dir)));
  assert(strstr(path + strlen(dir) + 1, "/") == NULL);
  assert(entries(dir) == 1);
  assert(path[strlen(dir) + 1] != '.');
  struct stat st;
  assert(stat(path, &st) == 0 && st.st_size == 12);
  assert(library_download(&a, &b, dir, second, sizeof(second), &t, err,
                          sizeof(err)));
  assert(strcmp(path, second));
  assert(entries(dir) == 2);
  mode = 1;
  assert(!library_download(&a, &b, dir, second, sizeof(second), &t, err,
                           sizeof(err)));
  assert(entries(dir) == 2);
  assert(strstr(err, "format"));
  mode = 2;
  assert(!library_download(&a, &b, dir, second, sizeof(second), &t, err,
                           sizeof(err)));
  assert(entries(dir) == 2);
  mode = 3;
  strcpy(b.format, "MOBI");
  assert(library_download(&a, &b, dir, second, sizeof(second), &t, err,
                          sizeof(err)));
  assert(entries(dir) == 3);
  strcpy(b.cover_url, "https://covers.example.org/cover.png");
  assert(library_cover(&a, &b, dir, &t));
  assert(b.cover_path[0] && entries(dir) == 4);
  int cover_calls = calls;
  assert(library_cover(&a, &b, dir, &t) && calls == cover_calls);
  int before = calls;
  strcpy(b.id, "../bad");
  assert(!library_download(&a, &b, dir, second, sizeof(second), &t, err,
                           sizeof(err)));
  assert(calls == before);
  DIR *d = opendir(dir);
  struct dirent *e;
  while ((e = readdir(d)))
    if (strcmp(e->d_name, ".") && strcmp(e->d_name, "..")) {
      char p[PATH_CAP];
      snprintf(p, sizeof(p), "%s/%s", dir, e->d_name);
      unlink(p);
    }
  closedir(d);
  assert(rmdir(dir) == 0);
  curl_global_cleanup();
  puts("Request encoding and download lifecycle tests passed.");
}
