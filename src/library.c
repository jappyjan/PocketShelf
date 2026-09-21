#define _POSIX_C_SOURCE 200809L
#include "library.h"
#include "cJSON.h"
#include <ctype.h>
#include <curl/curl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#define JSON_LIMIT (4 * 1024 * 1024)
#define BOOK_LIMIT ((size_t)-1)
typedef struct {
  char *data;
  size_t length;
  FILE *file;
  Transfer *transfer;
  size_t limit;
} Sink;
static void copy(char *to, size_t cap, const char *from) {
  snprintf(to, cap, "%s", from ? from : "");
}
static const cJSON *field(const cJSON *obj, const char *key) {
  return cJSON_GetObjectItemCaseSensitive(obj, key);
}
static const char *str(const cJSON *obj, const char *key) {
  const cJSON *v = field(obj, key);
  return cJSON_IsString(v) ? v->valuestring : "";
}
static void identifier(const cJSON *obj, const char *key, char *out, size_t n) {
  const cJSON *v = field(obj, key);
  if (cJSON_IsNumber(v))
    snprintf(out, n, "%.0f", v->valuedouble);
  else
    copy(out, n, cJSON_IsString(v) ? v->valuestring : "");
}
static int token(const char *s) {
  if (!*s)
    return 0;
  for (; *s; s++)
    if (!isalnum((unsigned char)*s) && *s != '-' && *s != '_')
      return 0;
  return 1;
}
int valid_base(const char *url) {
  if (strncmp(url, "https://", 8))
    return 0;
  const char *host = url + 8;
  size_t n = strlen(host);
  if (!n || n > 240 || host[0] == '.' || host[n - 1] == '.')
    return 0;
  for (size_t i = 0; i < n; i++)
    if (!isalnum((unsigned char)host[i]) && host[i] != '.' && host[i] != '-')
      return 0;
  return strchr(host, '.') != NULL;
}
static const char *formats[] = {
    "",        "epub", "pdf", "mobi",    "azw",  "azw3",    "prc", "fb2",
    "fb2.zip", "djvu", "doc", "docx",    "rtf",  "txt",     "htm", "html",
    "chm",     "cbr",  "cbz", "acsm",    "jpeg", "jpg",     "bmp", "png",
    "tiff",    "tif",  "mp3", "mp3.zip", "ogg",  "ogg.zip", "m4a", "m4b"};
const char *format_option(int index) {
  return index >= 0 && index < (int)(sizeof(formats) / sizeof(*formats))
             ? formats[index]
             : NULL;
}
int supported_format(const char *f) {
  for (int i = 1; format_option(i); i++)
    if (!strcasecmp(f, formats[i]))
      return 1;
  return 0;
}
void transfer_cancel(Transfer *t) { __sync_lock_test_and_set(&t->cancel, 1); }
int transfer_percent(Transfer *t) {
  return __sync_fetch_and_add(&t->percent, 0);
}
static int progress(void *ctx, double total, double now, double ut, double un) {
  (void)ut;
  (void)un;
  Transfer *t = ctx;
  if (total > 0)
    __sync_lock_test_and_set(&t->percent, (int)(now * 100 / total));
  return __sync_fetch_and_add(&t->cancel, 0) != 0;
}
static size_t receive(void *ptr, size_t size, size_t nmemb, void *ctx) {
  Sink *s = ctx;
  size_t n = size * nmemb;
  if (n > s->limit - s->length)
    return 0;
  if (s->file) {
    size_t written = fwrite(ptr, 1, n, s->file);
    s->length += written;
    return written;
  }
  char *p = realloc(s->data, s->length + n + 1);
  if (!p)
    return 0;
  s->data = p;
  memcpy(p + s->length, ptr, n);
  s->length += n;
  p[s->length] = 0;
  return n;
}
/* Authenticated requests may follow redirects only inside the configured HTTPS
   origin. Keep the cookie jar in memory for ordinary session handshakes.
   File URLs receive no account cookies and may follow HTTPS-only redirects. */
static int same_origin(const char *url, const char *base) {
  size_t n = strlen(base);
  return !strncasecmp(base, "https://", 8) && !strncasecmp(url, base, n) &&
         (url[n] == '/' || url[n] == '?' || url[n] == '#' || url[n] == '\0');
}
static int request(const Account *a, const char *url, const char *post,
                   Sink *sink, int file, char *error, size_t cap) {
  CURL *c = curl_easy_init();
  if (!c) {
    copy(error, cap, "Cannot start network request.");
    return 0;
  }
  char ce[CURL_ERROR_SIZE] = {0}, cookie[384], redirect_error[512] = {0};
  char current_url[URL_CAP];
  if (snprintf(current_url, sizeof(current_url), "%s", url) >=
      (int)sizeof(current_url)) {
    copy(error, cap, "The request address is too long.");
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
  curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, receive);
  curl_easy_setopt(c, CURLOPT_WRITEDATA, sink);
  curl_easy_setopt(c, CURLOPT_NOPROGRESS, 0L);
  curl_easy_setopt(c, CURLOPT_PROGRESSFUNCTION, progress);
  curl_easy_setopt(c, CURLOPT_PROGRESSDATA, sink->transfer);
  /* The default negotiated HTTP mode failed the live cookie handshake;
     HTTP/1.1 reached the JSON login endpoint in the anonymous probe. */
  curl_easy_setopt(c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
  if (!file) {
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "X-Requested-With: XMLHttpRequest");
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, headers);
    if (a->user[0] && a->key[0] && token(a->user) && token(a->key)) {
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
      copy(redirect_error, sizeof(redirect_error),
           "The server returned a redirect without a destination.");
      break;
    }
    if (!same_origin(next, a->base)) {
      copy(redirect_error, sizeof(redirect_error),
           "The site redirected to a different or insecure address. Update "
           "Library address on the sign-in form to a current HTTPS address you "
           "trust. Your credentials were not forwarded.");
      break;
    }
    if (hop >= 4) {
      copy(redirect_error, sizeof(redirect_error),
           "The site keeps redirecting this request (redirect loop). It may "
           "require a browser check.");
      break;
    }
    if (snprintf(current_url, sizeof(current_url), "%s", next) >=
        (int)sizeof(current_url)) {
      copy(redirect_error, sizeof(redirect_error),
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
      copy(error, cap, "Cancelled.");
    else if (redirect_error[0])
      copy(error, cap, redirect_error);
    else if (status == 401)
      copy(error, cap, "Please sign in again.");
    else if (status == 403 || status == 429 || status == 517)
      copy(error, cap,
           "The site blocked this request or requires a browser check. Try its "
           "website or another address you trust.");
    else if (rc == CURLE_WRITE_ERROR)
      copy(error, cap,
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
static int (*transport)(const Account *, const char *, const char *, Sink *,
                        int, char *, size_t) = request;

static cJSON *decode(const char *data, char *error, size_t cap) {
  cJSON *j = cJSON_Parse(data ? data : "");
  if (!cJSON_IsObject(j)) {
    cJSON_Delete(j);
    copy(error, cap,
         "The site returned an unexpected response (possibly a browser "
         "challenge).");
    return NULL;
  }
  const cJSON *e = field(j, "error"), *success = field(j, "success");
  if ((e && !cJSON_IsNull(e) && !cJSON_IsFalse(e)) ||
      (success && ((cJSON_IsNumber(success) && success->valueint == 0) ||
                   cJSON_IsFalse(success)))) {
    const char *message =
        cJSON_IsString(e) ? e->valuestring : str(e, "message");
    if (!*message)
      message = str(j, "message");
    copy(error, cap, *message ? message : "The library rejected this request.");
    cJSON_Delete(j);
    return NULL;
  }
  return j;
}
static cJSON *api(const Account *a, const char *path, const char *post,
                  Transfer *t, char *error, size_t cap) {
  if (!valid_base(a->base)) {
    copy(error, cap, "Set a valid HTTPS library address first.");
    return NULL;
  }
  char url[URL_CAP];
  snprintf(url, sizeof(url), "%s%s", a->base, path);
  Sink s = {.transfer = t, .limit = JSON_LIMIT};
  int ok = transport(a, url, post, &s, 0, error, cap);
  cJSON *j = ok ? decode(s.data, error, cap) : NULL;
  free(s.data);
  return j;
}
int parse_session(const char *json, Account *a, char *error, size_t cap) {
  cJSON *j = decode(json, error, cap);
  if (!j)
    return 0;
  const cJSON *s = field(j, "response");
  if (!cJSON_IsObject(s))
    s = field(j, "user");
  char user[64], key[256];
  identifier(s, "user_id", user, sizeof(user));
  if (!user[0])
    identifier(s, "id", user, sizeof(user));
  copy(key, sizeof(key), str(s, "user_key"));
  if (!key[0])
    copy(key, sizeof(key), str(s, "remix_userkey"));
  int ok = token(user) && token(key);
  if (ok) {
    copy(a->user, sizeof(a->user), user);
    copy(a->key, sizeof(a->key), key);
  } else
    copy(error, cap,
         *str(s, "message")
             ? str(s, "message")
             : "Sign-in failed. Check your account and the library address.");
  cJSON_Delete(j);
  return ok;
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
  cJSON *j = api(a, "/rpc.php", body, t, error, cap);
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
/* Convert the description's simple HTML to reader text, retaining paragraphs.
 */
static void plain_text(char *out, size_t cap, const char *in) {
  size_t n = 0;
  while (*in && n + 5 < cap) {
    if (*in == '<') {
      if (!strncasecmp(in, "<br", 3) || !strncasecmp(in, "</p", 3) ||
          !strncasecmp(in, "</div", 5))
        out[n++] = '\n';
      const char *end = strchr(in, '>');
      if (!end)
        break;
      in = end + 1;
    } else if (*in == '&') {
      const char *entities[] = {"&amp;",  "&lt;",   "&gt;",
                                "&quot;", "&apos;", "&nbsp;"};
      const char values[] = "&<>\"' ";
      int found = 0;
      for (int i = 0; i < 6; i++)
        if (!strncmp(in, entities[i], strlen(entities[i]))) {
          out[n++] = values[i];
          in += strlen(entities[i]);
          found = 1;
          break;
        }
      if (!found && in[1] == '#') {
        char *end;
        unsigned long c = strtoul(in + (in[2] == 'x' || in[2] == 'X' ? 3 : 2),
                                  &end, in[2] == 'x' || in[2] == 'X' ? 16 : 10);
        if (*end == ';' && c > 0 && c <= 0x10ffff &&
            !(c >= 0xd800 && c <= 0xdfff)) {
          if (c < 128)
            out[n++] = c;
          else if (c < 2048) {
            out[n++] = 0xc0 | (c >> 6);
            out[n++] = 0x80 | (c & 63);
          } else if (c < 65536) {
            out[n++] = 0xe0 | (c >> 12);
            out[n++] = 0x80 | ((c >> 6) & 63);
            out[n++] = 0x80 | (c & 63);
          } else {
            out[n++] = 0xf0 | (c >> 18);
            out[n++] = 0x80 | ((c >> 12) & 63);
            out[n++] = 0x80 | ((c >> 6) & 63);
            out[n++] = 0x80 | (c & 63);
          }
          in = end + 1;
          found = 1;
        }
      }
      if (!found)
        out[n++] = *in++;
    } else
      out[n++] = *in++;
  }
  out[n] = 0;
}
static void read_book(const cJSON *v, Book *b) {
  identifier(v, "id", b->id, sizeof(b->id));
  copy(b->hash, sizeof(b->hash), str(v, "hash"));
  plain_text(b->title, sizeof(b->title),
             *str(v, "title") ? str(v, "title") : "Untitled");
  plain_text(b->author, sizeof(b->author), str(v, "author"));
  copy(b->format, sizeof(b->format), str(v, "extension"));
  for (char *p = b->format; *p; p++)
    *p = tolower((unsigned char)*p);
  copy(b->language, sizeof(b->language), str(v, "language"));
  copy(b->size, sizeof(b->size), str(v, "filesizeString"));
  copy(b->cover_url, sizeof(b->cover_url), str(v, "cover"));
  plain_text(b->description, sizeof(b->description), str(v, "description"));
  const char *keys[] = {
      "author",         "year",       "publicationDate", "publisher",
      "edition",        "volume",     "series",          "pages",
      "identifier",     "isbn",       "language",        "extension",
      "filesizeString", "categories", "interestScore",   "qualityScore"};
  const char *labels[] = {"Author",
                          "Publication year",
                          "Publication date",
                          "Publisher",
                          "Edition",
                          "Volume",
                          "Series",
                          "Pages",
                          "ISBN / identifier",
                          "ISBN",
                          "Language",
                          "Format",
                          "File size",
                          "Categories",
                          "Reader rating",
                          "Quality rating"};
  b->metadata[0] = 0;
  for (size_t i = 0; i < sizeof(keys) / sizeof(*keys); i++) {
    char value[1024];
    identifier(v, keys[i], value, sizeof(value));
    if (*value) {
      size_t n = strlen(b->metadata);
      snprintf(b->metadata + n, sizeof(b->metadata) - n, "%s: %s\n", labels[i],
               value);
    }
  }
}
int parse_book_details(const char *json, Book *b, char *error, size_t cap) {
  cJSON *j = decode(json, error, cap);
  if (!j)
    return 0;
  const cJSON *v = field(j, "book");
  if (!cJSON_IsObject(v)) {
    copy(error, cap, "The library returned no book details.");
    cJSON_Delete(j);
    return 0;
  }
  Book next = {0};
  read_book(v, &next);
  if (!token(next.id) || !token(next.hash)) {
    copy(error, cap, "Invalid book details.");
    cJSON_Delete(j);
    return 0;
  }
  copy(next.cover_path, sizeof(next.cover_path), b->cover_path);
  *b = next;
  cJSON_Delete(j);
  return 1;
}
int library_details(const Account *a, Book *b, Transfer *t, char *error,
                    size_t cap) {
  if (!token(b->id) || !token(b->hash))
    return 0;
  char path[256];
  snprintf(path, sizeof(path), "/eapi/book/%s/%s", b->id, b->hash);
  cJSON *j = api(a, path, NULL, t, error, cap);
  if (!j)
    return 0;
  char *json = cJSON_PrintUnformatted(j);
  int ok = parse_book_details(json, b, error, cap);
  free(json);
  cJSON_Delete(j);
  return ok;
}
int library_cover(const Account *a, Book *b, const char *dir, Transfer *t) {
  if (!token(b->id) || !token(b->hash) || strncmp(b->cover_url, "https://", 8))
    return 0;
  char path[PATH_CAP], tmp[PATH_CAP], err[512];
  if (snprintf(path, sizeof(path), "%s/%s-%s.img", dir, b->id, b->hash) >=
      (int)sizeof(path))
    return 0;
  struct stat st;
  if (stat(path, &st) == 0 && st.st_size > 0) {
    copy(b->cover_path, sizeof(b->cover_path), path);
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
  int ok = transport(a, b->cover_url, NULL, &sink, 2, err, sizeof(err));
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
    copy(b->cover_path, sizeof(b->cover_path), path);
    return 1;
  }
  unlink(tmp);
  return 0;
}
int parse_books(const char *json, Results *out, char *error, size_t cap) {
  memset(out, 0, sizeof(*out));
  cJSON *j = decode(json, error, cap);
  if (!j)
    return 0;
  const cJSON *books = field(j, "books");
  if (!cJSON_IsArray(books))
    books = field(field(j, "exactMatch"), "books");
  if (!cJSON_IsArray(books)) {
    copy(error, cap, "The library response has no book list.");
    cJSON_Delete(j);
    return 0;
  }
  const cJSON *v = NULL;
  cJSON_ArrayForEach(v, books) {
    if (out->count == BOOKS_PER_PAGE) {
      out->has_more = 1;
      break;
    }
    Book b = {0};
    read_book(v, &b);
    if (!token(b.id) || !token(b.hash))
      continue;
    out->books[out->count++] = b;
  }
  if (out->count == BOOKS_PER_PAGE)
    out->has_more = 1;
  cJSON_Delete(j);
  return 1;
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
  cJSON *j = api(a, path, popular ? NULL : body, t, error, cap);
  curl_free(q);
  curl_easy_cleanup(c);
  if (!j)
    return 0;
  char *json = cJSON_PrintUnformatted(j);
  int ok = parse_books(json, out, error, cap);
  free(json);
  cJSON_Delete(j);
  return ok;
}
static int file_signature(FILE *f, const char *format) {
  unsigned char h[128] = {0};
  rewind(f);
  size_t n = fread(h, 1, sizeof(h), f);
  if (!n)
    return 0;
  if (!strcasecmp(format, "epub") || !strcasecmp(format, "docx") ||
      !strcasecmp(format, "cbz") || strstr(format, ".zip"))
    return n >= 4 && !memcmp(h, "PK\003\004", 4);
  if (!strcasecmp(format, "pdf"))
    return n >= 5 && !memcmp(h, "%PDF-", 5);
  if (!strcasecmp(format, "mobi") || !strcasecmp(format, "azw3"))
    return n >= 68 && !memcmp(h + 60, "BOOKMOBI", 8);
  /* Text, DRM licences and legacy containers have multiple valid encodings.
     Do not reject valid books based on an invented universal signature. */
  if (strcasecmp(format, "html") && strcasecmp(format, "htm") &&
      strcasecmp(format, "txt")) {
    size_t i = 0;
    while (i < n && isspace(h[i]))
      i++;
    if (i + 5 < n && (!strncasecmp((char *)h + i, "<html", 5) ||
                      !strncasecmp((char *)h + i, "<!doc", 5)))
      return 0;
  }
  return supported_format(format);
}
int library_download(const Account *a, const Book *b, const char *dir,
                     char *path, size_t path_cap, Transfer *t, char *error,
                     size_t cap) {
  if (!token(b->id) || !token(b->hash) || !supported_format(b->format)) {
    copy(error, cap,
         "This file type is not supported by the PocketBook reader.");
    return 0;
  }
  char endpoint[256];
  snprintf(endpoint, sizeof(endpoint), "/eapi/book/%s/%s/file", b->id, b->hash);
  cJSON *j = api(a, endpoint, NULL, t, error, cap);
  if (!j)
    return 0;
  const cJSON *file = field(j, "file");
  const char *link = str(file, "downloadLink");
  char url[URL_CAP];
  if (!*link) {
    copy(error, cap,
         "No download is available. Check your account quota on the website.");
    cJSON_Delete(j);
    return 0;
  }
  int len = link[0] == '/' && link[1] != '/'
                ? snprintf(url, sizeof(url), "%s%s", a->base, link)
                : snprintf(url, sizeof(url), "%s", link);
  cJSON_Delete(j);
  if (len < 0 || len >= (int)sizeof(url) || strncmp(url, "https://", 8)) {
    copy(error, cap,
         "The server returned an invalid or insecure download URL.");
    return 0;
  }
  char tmp[PATH_CAP];
  if (snprintf(tmp, sizeof(tmp), "%s/.download-XXXXXX", dir) >=
      (int)sizeof(tmp)) {
    copy(error, cap, "Download folder path is too long.");
    return 0;
  }
  int fd = mkstemp(tmp);
  if (fd < 0) {
    copy(error, cap,
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
  int ok = transport(a, url, NULL, &sink, 1, error, cap);
  if (ok && (fflush(f) != 0 || !file_signature(f, b->format))) {
    copy(error, cap,
         "The download is incomplete or is not the selected book format.");
    ok = 0;
  }
  if (fclose(f) != 0) {
    copy(error, cap, "Could not finish writing the book.");
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
    copy(name, sizeof(name), "Book");
  if (ok && snprintf(path, path_cap, "%s/%s-%s-%s.%s", dir, name, b->id,
                     tmp + strlen(tmp) - 6, b->format) >= (int)path_cap) {
    copy(error, cap, "Book filename is too long.");
    ok = 0;
  }
  if (ok) {
    int reserve = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (reserve < 0) {
      copy(error, cap, "Cannot reserve the book filename.");
      ok = 0;
    } else {
      close(reserve);
      if (rename(tmp, path) != 0) {
        unlink(path);
        copy(error, cap, "Cannot save the completed book.");
        ok = 0;
      }
    }
  }
  if (!ok)
    unlink(tmp);
  return ok;
}
