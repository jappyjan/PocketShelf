#include "storage/book_file.h"
#include "core/book.h"
#include <ctype.h>
#include <string.h>
#include <strings.h>
int book_file_valid(FILE *f, const char *format) {
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
