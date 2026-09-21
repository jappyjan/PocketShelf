#include "library/parser.h"
#include "core/book.h"
#include "core/text.h"
#include "library/json.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
static void read_book(const cJSON *v, Book *b) {
  json_identifier(v, "id", b->id, sizeof(b->id));
  text_copy(b->hash, sizeof(b->hash), json_string(v, "hash"));
  html_to_text(b->title, sizeof(b->title),
               *json_string(v, "title") ? json_string(v, "title") : "Untitled");
  html_to_text(b->author, sizeof(b->author), json_string(v, "author"));
  text_copy(b->format, sizeof(b->format), json_string(v, "extension"));
  for (char *p = b->format; *p; p++)
    *p = tolower((unsigned char)*p);
  text_copy(b->language, sizeof(b->language), json_string(v, "language"));
  text_copy(b->size, sizeof(b->size), json_string(v, "filesizeString"));
  text_copy(b->cover_url, sizeof(b->cover_url), json_string(v, "cover"));
  html_to_text(b->description, sizeof(b->description),
               json_string(v, "description"));
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
    json_identifier(v, keys[i], value, sizeof(value));
    if (*value) {
      size_t n = strlen(b->metadata);
      snprintf(b->metadata + n, sizeof(b->metadata) - n, "%s: %s\n", labels[i],
               value);
    }
  }
}

int parse_book_details(const char *json, Book *b, char *error, size_t cap) {
  cJSON *j = json_decode(json, error, cap);
  if (!j)
    return 0;
  const cJSON *v = json_field(j, "book");
  if (!cJSON_IsObject(v)) {
    text_copy(error, cap, "The library returned no book details.");
    cJSON_Delete(j);
    return 0;
  }
  Book next = {0};
  read_book(v, &next);
  if (!valid_identifier(next.id) || !valid_identifier(next.hash)) {
    text_copy(error, cap, "Invalid book details.");
    cJSON_Delete(j);
    return 0;
  }
  text_copy(next.cover_path, sizeof(next.cover_path), b->cover_path);
  *b = next;
  cJSON_Delete(j);
  return 1;
}

int parse_books(const char *json, Results *out, char *error, size_t cap) {
  memset(out, 0, sizeof(*out));
  cJSON *j = json_decode(json, error, cap);
  if (!j)
    return 0;
  const cJSON *books = json_field(j, "books");
  if (!cJSON_IsArray(books))
    books = json_field(json_field(j, "exactMatch"), "books");
  if (!cJSON_IsArray(books)) {
    text_copy(error, cap, "The library response has no book list.");
    cJSON_Delete(j);
    return 0;
  }
  const cJSON *v = NULL;
  cJSON_ArrayForEach(v, books) {
    if (out->count == MAX_BOOKS_PER_RESPONSE) {
      text_copy(error, cap,
                "The library returned too many books in one response.");
      cJSON_Delete(j);
      memset(out, 0, sizeof(*out));
      return 0;
    }
    Book b = {0};
    read_book(v, &b);
    if (!valid_identifier(b.id) || !valid_identifier(b.hash))
      continue;
    book_summarize(&b, &out->books[out->count++]);
  }
  if (out->count >= BOOKS_PER_PAGE)
    out->has_more = 1;
  const cJSON *pagination = json_field(j, "pagination");
  const cJSON *current = json_field(pagination, "current");
  const cJSON *total = json_field(pagination, "total_pages");
  if (cJSON_IsNumber(current) && cJSON_IsNumber(total)) {
    out->paginated = 1;
    out->has_more = out->count > 0 && current->valuedouble < total->valuedouble;
  }
  cJSON_Delete(j);
  return 1;
}

int parse_session(const char *json, Account *a, char *error, size_t cap) {
  cJSON *j = json_decode(json, error, cap);
  if (!j)
    return 0;
  const cJSON *s = json_field(j, "response");
  if (!cJSON_IsObject(s))
    s = json_field(j, "user");
  char user[64], key[256];
  json_identifier(s, "user_id", user, sizeof(user));
  if (!user[0])
    json_identifier(s, "id", user, sizeof(user));
  text_copy(key, sizeof(key), json_string(s, "user_key"));
  if (!key[0])
    text_copy(key, sizeof(key), json_string(s, "remix_userkey"));
  int ok = valid_identifier(user) && valid_identifier(key);
  if (ok) {
    text_copy(a->user, sizeof(a->user), user);
    text_copy(a->key, sizeof(a->key), key);
  } else
    text_copy(
        error, cap,
        *json_string(s, "message")
            ? json_string(s, "message")
            : "Sign-in failed. Check your account and the library address.");
  cJSON_Delete(j);
  return ok;
}
