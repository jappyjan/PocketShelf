#ifndef POCKETSHELF_SRC_CORE_TEXT_H
#define POCKETSHELF_SRC_CORE_TEXT_H
#include <stddef.h>
void text_copy(char *, size_t, const char *);
void secure_wipe(void *, size_t);
int valid_identifier(const char *);
int valid_base(const char *);
void html_to_text(char *, size_t, const char *);

#endif
