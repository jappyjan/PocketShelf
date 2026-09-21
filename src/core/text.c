#include "core/text.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
void text_copy(char *to, size_t cap, const char *from) {
  snprintf(to, cap, "%s", from ? from : "");
}

int valid_identifier(const char *s) {
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

void html_to_text(char *out, size_t cap, const char *in) {
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
void secure_wipe(void *p, size_t n) {
  volatile unsigned char *s = p;
  while (n--)
    *s++ = 0;
}
