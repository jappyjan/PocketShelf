#ifndef POCKETSHELF_SRC_CORE_MODELS_H
#define POCKETSHELF_SRC_CORE_MODELS_H
#include <stddef.h>
#define BOOKS_PER_PAGE 4
#define MAX_BOOKS_PER_RESPONSE 256
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
/* Compact list records: full description and metadata belong to book details.
 */
typedef struct {
  char id[64], hash[128], title[512], author[256], format[16], language[64],
      size[64], cover_url[URL_CAP], cover_path[PATH_CAP];
} BookSummary;
typedef struct {
  BookSummary books[MAX_BOOKS_PER_RESPONSE];
  int count, has_more, paginated;
} Results;
typedef struct {
  int cancel, percent;
} Transfer;

#endif
