#ifndef POCKETSHELF_SRC_LIBRARY_PARSER_H
#define POCKETSHELF_SRC_LIBRARY_PARSER_H
#include "core/models.h"
int parse_session(const char *, Account *, char *, size_t);
int parse_books(const char *, Results *, char *, size_t);
int parse_book_details(const char *, Book *, char *, size_t);

#endif
