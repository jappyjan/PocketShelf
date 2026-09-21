#include "library/json.h"
#include "core/text.h"
#include <stdio.h>
const cJSON *json_field(const cJSON *obj, const char *key) {
  return cJSON_GetObjectItemCaseSensitive(obj, key);
}

const char *json_string(const cJSON *obj, const char *key) {
  const cJSON *v = json_field(obj, key);
  return cJSON_IsString(v) ? v->valuestring : "";
}

void json_identifier(const cJSON *obj, const char *key, char *out, size_t n) {
  const cJSON *v = json_field(obj, key);
  if (cJSON_IsNumber(v))
    snprintf(out, n, "%.0f", v->valuedouble);
  else
    text_copy(out, n, cJSON_IsString(v) ? v->valuestring : "");
}

cJSON *json_decode(const char *data, char *error, size_t cap) {
  cJSON *j = cJSON_Parse(data ? data : "");
  if (!cJSON_IsObject(j)) {
    cJSON_Delete(j);
    text_copy(error, cap,
              "The site returned an unexpected response (possibly a browser "
              "challenge).");
    return NULL;
  }
  const cJSON *e = json_field(j, "error"), *success = json_field(j, "success");
  if ((e && !cJSON_IsNull(e) && !cJSON_IsFalse(e)) ||
      (success && ((cJSON_IsNumber(success) && success->valueint == 0) ||
                   cJSON_IsFalse(success)))) {
    const char *message =
        cJSON_IsString(e) ? e->valuestring : json_string(e, "message");
    if (!*message)
      message = json_string(j, "message");
    text_copy(error, cap,
              *message ? message : "The library rejected this request.");
    cJSON_Delete(j);
    return NULL;
  }
  return j;
}
