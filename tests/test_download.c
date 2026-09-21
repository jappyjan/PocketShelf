#include "../src/library.c"
#include <assert.h>
#include <dirent.h>
static int calls, mode;
static char payload[4096];
static int fixture(const Account *a, const char *url, const char *post,
                   Sink *sink, int file, char *error, size_t cap) {
  (void)a;
  calls++;
  if (file == 2) {
    assert(!strcmp(url, "https://covers.example.org/cover.png"));
    const unsigned char png[] = {137, 80, 78, 71, 13, 10, 26, 10};
    return receive((void *)png, 1, sizeof(png), sink) == sizeof(png);
  }
  if (post)
    snprintf(payload, sizeof(payload), "%s", post);
  const char *body = "";
  if (strstr(url, "/rpc.php"))
    body = "{\"response\":{\"user_id\":123,\"user_key\":\"session\"}}";
  else if (strstr(url, "/search"))
    body = "{\"books\":[]}";
  else if (!file) {
    assert(strstr(url, "/eapi/book/123/abc/file"));
    body = "{\"success\":1,\"file\":{\"downloadLink\":\"https://"
           "files.example.org/book\"}}";
  } else {
    assert(!strcmp(url, "https://files.example.org/book"));
    if (mode == 2) {
      receive("PK\003\004partial", 1, 11, sink);
      copy(error, cap, "Connection lost.");
      return 0;
    }
    if (mode == 3) {
      unsigned char mobi[80] = {0};
      memcpy(mobi + 60, "BOOKMOBI", 8);
      return receive(mobi, 1, sizeof(mobi), sink) == sizeof(mobi);
    }
    body = mode == 1 ? "<html>Please login</html>" : "PK\003\004testbook";
  }
  return receive((void *)body, 1, strlen(body), sink) == strlen(body);
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
  transport = fixture;
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
