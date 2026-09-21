#include <curl/curl.h>
static const char *test_ca;
static CURLcode trusted_test_perform(CURL *c);
#define curl_easy_perform trusted_test_perform
#include "../src/library.c"
#undef curl_easy_perform
#include <assert.h>
static CURLcode trusted_test_perform(CURL *c) {
  curl_easy_setopt(c, CURLOPT_CAINFO, test_ca);
  curl_easy_setopt(c, CURLOPT_NOPROXY, "localhost,127.0.0.1");
  return curl_easy_perform(c);
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
