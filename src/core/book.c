#include "core/book.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
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
void book_overview(const Book *b, char *out, size_t cap) {
  const char *fields[] = {b->author, b->format, b->language};
  if (!cap)
    return;
  out[0] = 0;
  for (size_t i = 0; i < sizeof(fields) / sizeof(*fields); i++) {
    const char *value = fields[i];
    while (isspace((unsigned char)*value))
      value++;
    size_t length = strlen(value);
    while (length && isspace((unsigned char)value[length - 1]))
      length--;
    if (!length)
      continue;
    size_t used = strlen(out);
    snprintf(out + used, cap - used, "%s%.*s", used ? " | " : "", (int)length,
             value);
  }
}

/* The compact record is the prefix of Book. Keep layout assumptions here. */
typedef char
    SummaryLayoutCheck[sizeof(BookSummary) == offsetof(Book, description) ? 1
                                                                          : -1];
void book_summarize(const Book *book, BookSummary *summary) {
  memcpy(summary, book, sizeof(*summary));
}
void book_expand(const BookSummary *summary, Book *book) {
  memset(book, 0, sizeof(*book));
  memcpy(book, summary, sizeof(*summary));
}
