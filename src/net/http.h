#ifndef POCKETSHELF_SRC_NET_HTTP_H
#define POCKETSHELF_SRC_NET_HTTP_H
#include "net/transfer.h"
/* A NULL options argument uses the system TLS trust store and proxy settings.
 */
typedef struct {
  const char *ca_file, *no_proxy;
} HttpOptions;
int http_request(const Account *, const char *, const char *, Sink *, int,
                 char *, size_t);
int http_request_configured(const Account *, const char *, const char *, Sink *,
                            int, char *, size_t, const HttpOptions *);
int http_same_origin(const char *, const char *);

#endif
