#include "cJSON.h"
#include "core/book.h"
#include "core/text.h"
#include "library/parser.h"
#include "net/http.h"
#include "storage/book_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>
static int checks;
#define CHECK(x)                                                               \
  do {                                                                         \
    assert(x);                                                                 \
    checks++;                                                                  \
  } while (0)
int main(void) {
  char error[512] = {0};
  Results r;
  Account a = {0};
  CHECK(valid_base("https://z-library.sk"));
  CHECK(!valid_base("http://z-library.sk"));
  CHECK(!valid_base("https://user:password@example.org"));
  CHECK(!valid_base("https://example.org/path"));
  CHECK(!valid_base("https://example.org\r\nCookie:bad"));
  CHECK(!valid_base("https://"));
  CHECK(
      parse_session("{\"response\":{\"user_id\":123,\"user_key\":\"abc123\"}}",
                    &a, error, sizeof(error)));
  CHECK(!strcmp(a.user, "123") && !strcmp(a.key, "abc123"));
  CHECK(parse_session("{\"user\":{\"id\":\"456\",\"remix_userkey\":\"xyz\"}}",
                      &a, error, sizeof(error)));
  CHECK(!parse_session("{\"response\":{\"validationError\":true,\"message\":"
                       "\"Incorrect email or password\"}}",
                       &a, error, sizeof(error)));
  CHECK(strstr(error, "Incorrect email") != NULL);
  CHECK(!strcmp(a.user, "456"));
  CHECK(!parse_session(
      "{\"response\":{\"user_id\":123,\"user_key\":\"abc; bad=cookie\"}}", &a,
      error, sizeof(error)));
  CHECK(!parse_session("{\"response\":null}", &a, error, sizeof(error)));
  CHECK(
      !parse_books("<html>Browser challenge</html>", &r, error, sizeof(error)));
  CHECK(!parse_books("null", &r, error, sizeof(error)));
  CHECK(!parse_books("{\"success\":0,\"message\":\"Please login\"}", &r, error,
                     sizeof(error)));
  CHECK(!strcmp(error, "Please login"));
  CHECK(!parse_books("{\"error\":{\"message\":\"Quota reached\"}}", &r, error,
                     sizeof(error)));
  CHECK(!strcmp(error, "Quota reached"));
  CHECK(parse_books("{\"books\":[]}", &r, error, sizeof(error)) &&
        r.count == 0 && !r.has_more);
  CHECK(parse_books(
      "{\"exactMatch\":{\"books\":[{\"id\":1,\"hash\":\"abc\",\"title\":\"Café "
      "世界\",\"author\":null,\"extension\":\"epub\"}]}}",
      &r, error, sizeof(error)));
  CHECK(r.count == 1 && !strcmp(r.books[0].title, "Café 世界") &&
        !r.books[0].author[0]);
  CHECK(parse_books(
      "{\"books\":[{\"id\":\"../"
      "x\",\"hash\":\"abc\"},{\"id\":1,\"hash\":\"abc\",\"title\":null}]}",
      &r, error, sizeof(error)));
  CHECK(r.count == 1 && !strcmp(r.books[0].title, "Untitled"));
  cJSON *root = cJSON_CreateObject(),
        *array = cJSON_AddArrayToObject(root, "books");
  for (int i = 0; i < 20; i++) {
    cJSON *b = cJSON_CreateObject();
    cJSON_AddNumberToObject(b, "id", i + 1);
    cJSON_AddStringToObject(b, "hash", "abc");
    cJSON_AddItemToArray(array, b);
  }
  char *data = cJSON_PrintUnformatted(root);
  CHECK(parse_books(data, &r, error, sizeof(error)));
  CHECK(r.count == 20 &&
        r.has_more); /* Never truncate a server response to UI page size. */
  free(data);
  cJSON_Delete(root);
  Transfer t = {0};
  Sink sink = {.transfer = &t, .limit = 5};
  CHECK(sink_write("123", 1, 3, &sink) == 3);
  CHECK(sink_write("456", 1, 3, &sink) == 0);
  CHECK(!strcmp(sink.data, "123"));
  free(sink.data);
  CHECK(transfer_progress(&t, 100, 37, 0, 0) == 0 &&
        transfer_percent(&t) == 37);
  transfer_cancel(&t);
  CHECK(transfer_progress(&t, 100, 40, 0, 0) == 1);
  FILE *f = tmpfile();
  CHECK(f != NULL);
  fwrite("<html>no book</html>", 1, 20, f);
  CHECK(!book_file_valid(f, "epub") && !book_file_valid(f, "pdf"));
  fclose(f);
  f = tmpfile();
  fwrite("%PDF-1.7", 1, 8, f);
  CHECK(book_file_valid(f, "pdf"));
  CHECK(!book_file_valid(f, "epub"));
  fclose(f);
  f = tmpfile();
  fwrite("PK\003\004xxxx", 1, 8, f);
  CHECK(book_file_valid(f, "epub"));
  fclose(f);
  CHECK(supported_format("MOBI"));
  CHECK(supported_format("azw3") && supported_format("fb2.zip") &&
        supported_format("djvu"));
  CHECK(supported_format("epub") && supported_format("pdf") &&
        !supported_format("app"));
  Book detail = {0};
  CHECK(parse_book_details(
      "{\"book\":{\"id\":123,\"hash\":\"abc\",\"title\":\"1984\",\"extension\":"
      "\"MOBI\",\"year\":1949,\"pages\":328,\"publisher\":\"Secker\","
      "\"identifier\":\"978-test\",\"description\":\"<p>First &amp; "
      "second.</p><p>&#x4e16;&#30028;</p>\",\"cover\":\"https://example.org/"
      "cover.jpg\"}}",
      &detail, error, sizeof(error)));
  CHECK(!strcmp(detail.format, "mobi") && supported_format(detail.format));
  CHECK(strstr(detail.metadata, "Publication year: 1949") &&
        strstr(detail.metadata, "Pages: 328") &&
        strstr(detail.metadata, "ISBN / identifier: 978-test"));
  CHECK(!strcmp(detail.description, "First & second.\n世界\n"));
  CHECK(!strcmp(detail.cover_url, "https://example.org/cover.jpg"));
  CHECK(!parse_book_details("{\"book\":null}", &detail, error, sizeof(error)));
  for (int i = 1; format_option(i); i++)
    CHECK(supported_format(format_option(i)));
  f = tmpfile();
  unsigned char mobi[80] = {0};
  memcpy(mobi + 60, "BOOKMOBI", 8);
  fwrite(mobi, 1, sizeof(mobi), f);
  CHECK(book_file_valid(f, "MOBI") && book_file_valid(f, "azw3"));
  fclose(f);
  puts("Library tests passed.");
  printf("%d checks\n", checks);
  return 0;
}
