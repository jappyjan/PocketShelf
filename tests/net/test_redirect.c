#include "cJSON.h"
#include "core/book.h"
#include "core/text.h"
#include "library/parser.h"
#include "net/http.h"
#include "storage/book_file.h"
#include <assert.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char *test_ca;
static int request(const Account *a, const char *url, const char *post,
                   Sink *sink, int file, char *error, size_t cap) {
  HttpOptions options = {.ca_file = test_ca, .no_proxy = "localhost,127.0.0.1"};
  return http_request_configured(a, url, post, sink, file, error, cap,
                                 &options);
}
int main(int argc, char **argv) {
  assert(argc == 3);
  test_ca = argv[2];
  curl_global_init(CURL_GLOBAL_DEFAULT);
  Account a = {.user = "123", .key = "fixture-session"};
  snprintf(a.base, sizeof(a.base), "%s", argv[1]);
  Transfer t = {0};
  Sink s = {.transfer = &t, .limit = JSON_LIMIT};
  char url[URL_CAP], error[512];
  snprintf(url, sizeof(url), "%s/login", a.base);
  assert(request(&a, url, "password=fixture-password", &s, 0, error,
                 sizeof(error)) &&
         "A same-origin 307 cookie handshake must complete");
  assert(!strcmp(s.data, "{\"success\":1}"));
  free(s.data);
  s = (Sink){.transfer = &t, .limit = JSON_LIMIT};
  snprintf(url, sizeof(url), "%s/cross-host", a.base);
  assert(!request(&a, url, "password=fixture-password", &s, 0, error,
                  sizeof(error)));
  assert(strstr(error, "different"));
  free(s.data);
  s = (Sink){.transfer = &t, .limit = JSON_LIMIT};
  snprintf(url, sizeof(url), "%s/downgrade", a.base);
  assert(!request(&a, url, "password=fixture-password", &s, 0, error,
                  sizeof(error)));
  free(s.data);
  s = (Sink){.transfer = &t, .limit = JSON_LIMIT};
  snprintf(url, sizeof(url), "%s/loop", a.base);
  assert(!request(&a, url, "password=fixture-password", &s, 0, error,
                  sizeof(error)));
  assert(strstr(error, "loop") || strstr(error, "repeated"));
  free(s.data);
  curl_global_cleanup();
  puts("HTTPS cookie redirect, origin isolation and loop-limit tests passed.");
}
