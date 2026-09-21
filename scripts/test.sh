#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build
cc -D_DARWIN_C_SOURCE -std=c99 -Wall -Wextra -Wno-deprecated-declarations -fsanitize=address,undefined -g \
  -Isrc -Ivendor tests/test_library.c vendor/cJSON.c -lcurl -lm -o build/test_library
./build/test_library
cc -D_DARWIN_C_SOURCE -std=c99 -Wall -Wextra -Wno-deprecated-declarations -fsanitize=address,undefined -g \
  -Isrc -Ivendor tests/test_download.c vendor/cJSON.c -lcurl -lm -o build/test_download
./build/test_download

./scripts/test-signin.sh

./scripts/test-redirect.sh
