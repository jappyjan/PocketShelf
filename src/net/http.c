#include "net/http.h"
#include "core/text.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
int http_same_origin(const char *url, const char *base) {
  size_t n = strlen(base);
  return !strncasecmp(base, "https://", 8) && !strncasecmp(url, base, n) &&
         (url[n] == '/' || url[n] == '?' || url[n] == '#' || url[n] == '\0');
}

int http_request_configured(const Account *a, const char *url, const char *post,
                            Sink *sink, int file, char *error, size_t cap,
                            const HttpOptions *options) {
  CURL *c = curl_easy_init();
  if (!c) {
    text_copy(error, cap, "Cannot start network request.");
    return 0;
  }
  if (options && options->ca_file)
    curl_easy_setopt(c, CURLOPT_CAINFO, options->ca_file);
  if (options && options->no_proxy)
    curl_easy_setopt(c, CURLOPT_NOPROXY, options->no_proxy);
  char ce[CURL_ERROR_SIZE] = {0}, cookie[384], redirect_error[512] = {0};
  char current_url[URL_CAP];
  if (snprintf(current_url, sizeof(current_url), "%s", url) >=
      (int)sizeof(current_url)) {
    text_copy(error, cap, "The request address is too long.");
    curl_easy_cleanup(c);
    return 0;
  }
  struct curl_slist *headers = NULL;
  curl_easy_setopt(c, CURLOPT_URL, current_url);
  curl_easy_setopt(c, CURLOPT_COOKIEFILE, "");
  curl_easy_setopt(c, CURLOPT_PROTOCOLS, (long)CURLPROTO_HTTPS);
  curl_easy_setopt(c, CURLOPT_REDIR_PROTOCOLS, (long)CURLPROTO_HTTPS);
  curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, (long)file);
  curl_easy_setopt(c, CURLOPT_MAXREDIRS, 5L);
  curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(c, CURLOPT_SSL_VERIFYHOST, 2L);
  curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT, 20L);
  curl_easy_setopt(c, CURLOPT_TIMEOUT, file == 2 ? 10L : file ? 0L : 45L);
  curl_easy_setopt(c, CURLOPT_LOW_SPEED_LIMIT, 100L);
  curl_easy_setopt(c, CURLOPT_LOW_SPEED_TIME, 30L);
  curl_easy_setopt(c, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(c, CURLOPT_ERRORBUFFER, ce);
  curl_easy_setopt(c, CURLOPT_USERAGENT, "PocketShelf/0.1");
  curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, sink_write);
  curl_easy_setopt(c, CURLOPT_WRITEDATA, sink);
  curl_easy_setopt(c, CURLOPT_NOPROGRESS, 0L);
  curl_easy_setopt(c, CURLOPT_PROGRESSFUNCTION, transfer_progress);
  curl_easy_setopt(c, CURLOPT_PROGRESSDATA, sink->transfer);
  /* The default negotiated HTTP mode failed the live cookie handshake;
     HTTP/1.1 reached the JSON login endpoint in the anonymous probe. */
  curl_easy_setopt(c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
  if (!file) {
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "X-Requested-With: XMLHttpRequest");
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, headers);
    if (a->user[0] && a->key[0] && valid_identifier(a->user) &&
        valid_identifier(a->key)) {
      snprintf(cookie, sizeof(cookie), "remix_userid=%s; remix_userkey=%s",
               a->user, a->key);
      curl_easy_setopt(c, CURLOPT_COOKIE, cookie);
    }
  }
  if (post)
    curl_easy_setopt(c, CURLOPT_POSTFIELDS, post);
  CURLcode rc;
  long status = 0;
  for (int hop = 0;; hop++) {
    ce[0] = 0;
    rc = curl_easy_perform(c);
    curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &status);
    fprintf(stderr,
            "[pocketshelf] http_result file=%d status=%ld curl=%d hop=%d\n",
            file, status, (int)rc, hop);
    fflush(stderr);
    if (file || rc != CURLE_OK || status < 300 || status >= 400)
      break;
    char *next = NULL;
    curl_easy_getinfo(c, CURLINFO_REDIRECT_URL, &next);
    if (!next || !*next) {
      text_copy(redirect_error, sizeof(redirect_error),
                "The server returned a redirect without a destination.");
      break;
    }
    if (!http_same_origin(next, a->base)) {
      text_copy(
          redirect_error, sizeof(redirect_error),
          "The site redirected to a different or insecure address. Update "
          "Library address on the sign-in form to a current HTTPS address you "
          "trust. Your credentials were not forwarded.");
      break;
    }
    if (hop >= 4) {
      text_copy(
          redirect_error, sizeof(redirect_error),
          "The site keeps redirecting this request (redirect loop). It may "
          "require a browser check.");
      break;
    }
    if (snprintf(current_url, sizeof(current_url), "%s", next) >=
        (int)sizeof(current_url)) {
      text_copy(redirect_error, sizeof(redirect_error),
                "The redirect address is too long.");
      break;
    }
    /* 307/308 preserve the request body. Other POST redirects become GET,
       matching browser semantics without replaying a password to a new page. */
    if (post && (status == 301 || status == 302 || status == 303)) {
      curl_easy_setopt(c, CURLOPT_HTTPGET, 1L);
      post = NULL;
    }
    free(sink->data);
    sink->data = NULL;
    sink->length = 0;
    __sync_lock_test_and_set(&sink->transfer->percent, 0);
    curl_easy_setopt(c, CURLOPT_URL, current_url);
  }
  int ok = rc == CURLE_OK && status >= 200 && status < 300;
  if (!ok) {
    if (__sync_fetch_and_add(&sink->transfer->cancel, 0))
      text_copy(error, cap, "Cancelled.");
    else if (redirect_error[0])
      text_copy(error, cap, redirect_error);
    else if (status == 401)
      text_copy(error, cap, "Please sign in again.");
    else if (status == 403 || status == 429 || status == 517)
      text_copy(
          error, cap,
          "The site blocked this request or requires a browser check. Try its "
          "website or another address you trust.");
    else if (rc == CURLE_WRITE_ERROR)
      text_copy(error, cap,
                "Response is too large, storage is full, or writing failed.");
    else if (rc != CURLE_OK)
      snprintf(error, cap, "Network/TLS error: %s",
               ce[0] ? ce : curl_easy_strerror(rc));
    else
      snprintf(error, cap, "Server returned HTTP %ld. Try again later.",
               status);
  }
  curl_slist_free_all(headers);
  curl_easy_cleanup(c);
  return ok;
}

int http_request(const Account *a, const char *url, const char *post,
                 Sink *sink, int file, char *error, size_t cap) {
  return http_request_configured(a, url, post, sink, file, error, cap, NULL);
}
