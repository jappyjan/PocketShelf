#ifndef LIBRARY_H
#define LIBRARY_H
#include <stddef.h>
#define BOOKS_PER_PAGE 4
#define URL_CAP 2048
#define PATH_CAP 1024
typedef struct {
  char base[256], user[64], key[256], email[256];
} Account;
typedef struct {
  char id[64], hash[128], title[512], author[256], format[16], language[64],
      size[64], cover_url[URL_CAP], cover_path[PATH_CAP], description[32768],
      metadata[8192];
} Book;
typedef struct {
  Book books[BOOKS_PER_PAGE];
  int count, has_more;
} Results;
typedef struct {
  int cancel, percent;
} Transfer;
int valid_base(const char *url);
int parse_books(const char *json, Results *out, char *error, size_t cap);
int parse_session(const char *json, Account *account, char *error, size_t cap);
int library_login(Account *, const char *, const char *, Transfer *, char *,
                  size_t);
int library_search(const Account *, const char *, const char *, int, int,
                   Results *, Transfer *, char *, size_t);
int library_download(const Account *, const Book *, const char *, char *,
                     size_t, Transfer *, char *, size_t);
void transfer_cancel(Transfer *t);
int transfer_percent(Transfer *t);
const char *format_option(int index);
int library_details(const Account *, Book *, Transfer *, char *, size_t);
int parse_book_details(const char *, Book *, char *, size_t);
int library_cover(const Account *, Book *, const char *, Transfer *);
int supported_format(const char *format);
#endif
