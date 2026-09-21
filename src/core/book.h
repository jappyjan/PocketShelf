#ifndef POCKETSHELF_SRC_CORE_BOOK_H
#define POCKETSHELF_SRC_CORE_BOOK_H
#include "core/models.h"
const char *format_option(int index);
int supported_format(const char *format);
void book_overview(const Book *, char *, size_t);
void book_summarize(const Book *, BookSummary *);
void book_expand(const BookSummary *, Book *);

#endif
