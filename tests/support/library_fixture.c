#include "support/app_test.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
LibraryFixture fixture = {.search_end = 20};
int library_login(Account *a, const char *e, const char *p, Transfer *t,
                  char *error, size_t cap) {
  (void)a;
  (void)t;
  assert(!strcmp(e, "reader@example.org"));
  assert(!strcmp(p, "secret-test-password"));
  if (fixture.success) {
    snprintf(a->user, sizeof(a->user), "123");
    snprintf(a->key, sizeof(a->key), "fixture-session");
    return 1;
  }
  snprintf(error, cap,
           "The site blocked this request or requires a browser check.");
  return 0;
}

int library_search(const Account *a, const char *q, const char *f, int p,
                   int popular_books, Results *r, Transfer *t, char *e,
                   size_t n) {
  (void)a;
  (void)f;
  (void)popular_books;
  __sync_fetch_and_add(&fixture.search_calls, 1);
  while (__sync_fetch_and_add(&fixture.hold_search, 0) &&
         !__sync_fetch_and_add(&t->cancel, 0)) {
    struct timespec delay = {0, 1000000};
    nanosleep(&delay, NULL);
  }
  if (fixture.search_fail || __sync_fetch_and_add(&t->cancel, 0)) {
    snprintf(e, n, "Offline");
    return 0;
  }
  memset(r, 0, sizeof(*r));
  r->count = popular_books ? 100 : BOOKS_PER_PAGE;
  r->has_more = !popular_books && p < fixture.search_end;
  for (int i = 0; i < r->count; i++) {
    snprintf(r->books[i].id, sizeof(r->books[i].id), "%d",
             (p - 1) * BOOKS_PER_PAGE + i);
    snprintf(r->books[i].title, sizeof(r->books[i].title), "%s Book %d", q, i);
    if (popular_books)
      strcpy(r->books[i].cover_url, "https://example.org/cover.png");
  }
  return 1;
}

int library_cover(const Account *a, Book *b, const char *dir, Transfer *t) {
  (void)a;
  (void)dir;
  (void)t;
  fixture.cover_calls++;
  strcpy(b->cover_path, "/tmp/fixture-cover.png");
  return 1;
}

int library_details(const Account *a, Book *b, Transfer *t, char *error,
                    size_t cap) {
  (void)a;
  (void)b;
  (void)t;
  (void)error;
  (void)cap;
  return 1;
}
int library_download(const Account *a, const Book *b, const char *dir,
                     char *path, size_t path_cap, Transfer *t, char *error,
                     size_t cap) {
  (void)a;
  (void)b;
  (void)dir;
  (void)path;
  (void)path_cap;
  (void)t;
  snprintf(error, cap, "Fixture download unavailable");
  return 0;
}
