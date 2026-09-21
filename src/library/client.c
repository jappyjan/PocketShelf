#include "core/text.h"
#include "library/internal.h"
#include "library/parser.h"
#include "net/http.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
cJSON *library_api(const Account *a, const char *path, const char *post,
                   Transfer *t, char *error, size_t cap) {
  if (!valid_base(a->base)) {
    text_copy(error, cap, "Set a valid HTTPS library address first.");
    return NULL;
  }
  char url[URL_CAP];
  snprintf(url, sizeof(url), "%s%s", a->base, path);
  Sink s = {.transfer = t, .limit = JSON_LIMIT};
  int ok = http_request(a, url, post, &s, 0, error, cap);
  cJSON *j = ok ? json_decode(s.data, error, cap) : NULL;
  free(s.data);
  return j;
}

int library_login(Account *a, const char *email, const char *password,
                  Transfer *t, char *error, size_t cap) {
  CURL *c = curl_easy_init();
  if (!c)
    return 0;
  char *e = curl_easy_escape(c, email, 0),
       *p = curl_easy_escape(c, password, 0),
       *b = curl_easy_escape(c, a->base, 0);
  if (!e || !p || !b) {
    curl_free(e);
    curl_free(p);
    curl_free(b);
    curl_easy_cleanup(c);
    return 0;
  }
  size_t n = strlen(e) + strlen(p) + strlen(b) + 160;
  char *body = malloc(n);
  if (!body) {
    curl_free(e);
    curl_free(p);
    curl_free(b);
    curl_easy_cleanup(c);
    return 0;
  }
  snprintf(body, n,
           "email=%s&password=%s&action=login&isModal=true&site_mode=books&gg_"
           "json_mode=1&redirectUrl=%s%%2F",
           e, p, b);
  cJSON *j = library_api(a, "/rpc.php", body, t, error, cap);
  memset(body, 0, n);
  free(body);
  curl_free(e);
  curl_free(p);
  curl_free(b);
  curl_easy_cleanup(c);
  if (!j)
    return 0;
  char *json = cJSON_PrintUnformatted(j);
  int ok = parse_session(json, a, error, cap);
  free(json);
  cJSON_Delete(j);
  return ok;
}

int library_details(const Account *a, Book *b, Transfer *t, char *error,
                    size_t cap) {
  if (!valid_identifier(b->id) || !valid_identifier(b->hash))
    return 0;
  char path[256];
  snprintf(path, sizeof(path), "/eapi/book/%s/%s", b->id, b->hash);
  cJSON *j = library_api(a, path, NULL, t, error, cap);
  if (!j)
    return 0;
  char *json = cJSON_PrintUnformatted(j);
  int ok = parse_book_details(json, b, error, cap);
  free(json);
  cJSON_Delete(j);
  return ok;
}

int library_search(const Account *a, const char *query, const char *format,
                   int page, int popular, Results *out, Transfer *t,
                   char *error, size_t cap) {
  CURL *c = curl_easy_init();
  if (!c)
    return 0;
  char *q = curl_easy_escape(c, query, 0);
  if (!q) {
    curl_easy_cleanup(c);
    return 0;
  }
  char body[4096], path[128];
  snprintf(body, sizeof(body), "message=%s&page=%d&limit=%d%s%s", q, page,
           BOOKS_PER_PAGE, *format ? "&extensions%5B0%5D=" : "", format);
  snprintf(path, sizeof(path),
           popular ? "/eapi/book/most-popular?page=%d&limit=%d"
                   : "/eapi/book/search",
           page, BOOKS_PER_PAGE);
  cJSON *j = library_api(a, path, popular ? NULL : body, t, error, cap);
  curl_free(q);
  curl_easy_cleanup(c);
  if (!j)
    return 0;
  char *json = cJSON_PrintUnformatted(j);
  int ok = parse_books(json, out, error, cap);
  free(json);
  cJSON_Delete(j);
  /* Popular is one complete collection on the current API. Do not invent
     another page unless the server explicitly supplies pagination. */
  if (ok && popular && !out->paginated)
    out->has_more = 0;
  return ok;
}
