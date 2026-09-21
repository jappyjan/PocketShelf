#include "storage/account.h"
#include "cJSON.h"
#include "core/text.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
int account_save(const char *path, const Account *account) {
  char temporary[PATH_CAP];
  snprintf(temporary, sizeof(temporary), "%s.tmp", path);
  cJSON *j = cJSON_CreateObject();
  if (!j)
    return 0;
  cJSON_AddStringToObject(j, "base", account->base);
  cJSON_AddStringToObject(j, "user", account->user);
  cJSON_AddStringToObject(j, "key", account->key);
  cJSON_AddStringToObject(j, "email", account->email);
  char *json = cJSON_PrintUnformatted(j);
  cJSON_Delete(j);
  if (!json)
    return 0;
  FILE *f = fopen(temporary, "w");
  int ok = 0;
  if (f) {
    chmod(temporary, 0600);
    ok = fputs(json, f) >= 0;
    if (fclose(f) != 0)
      ok = 0;
    if (ok)
      ok = rename(temporary, path) == 0;
  }
  if (!ok)
    unlink(temporary);
  free(json);
  return ok;
}
void account_load(const char *path, Account *account) {
  FILE *f = fopen(path, "r");
  if (!f)
    return;
  char data[2048];
  size_t n = fread(data, 1, sizeof(data) - 1, f);
  fclose(f);
  data[n] = 0;
  cJSON *j = cJSON_Parse(data);
  if (!j)
    return;
  cJSON *base = cJSON_GetObjectItemCaseSensitive(j, "base"),
        *user = cJSON_GetObjectItemCaseSensitive(j, "user"),
        *key = cJSON_GetObjectItemCaseSensitive(j, "key"),
        *saved_email = cJSON_GetObjectItemCaseSensitive(j, "email");
  if (cJSON_IsString(base) && valid_base(base->valuestring)) {
    snprintf(account->base, sizeof(account->base), "%s", base->valuestring);
    if (cJSON_IsString(user))
      snprintf(account->user, sizeof(account->user), "%s", user->valuestring);
    if (cJSON_IsString(key))
      snprintf(account->key, sizeof(account->key), "%s", key->valuestring);
  }
  if (cJSON_IsString(saved_email)) {
    snprintf(account->email, sizeof(account->email), "%s",
             saved_email->valuestring);
  }
  cJSON_Delete(j);
}

void account_forget(const char *path) {
  char temporary[PATH_CAP];
  snprintf(temporary, sizeof(temporary), "%s.tmp", path);
  unlink(path);
  unlink(temporary);
}
