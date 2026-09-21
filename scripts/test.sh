#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
. scripts/sources.sh
python3 scripts/check-architecture.py
mkdir -p build
cc -D_DARWIN_C_SOURCE -D_POSIX_C_SOURCE=200809L -std=c99 -Wall -Wextra -Wno-deprecated-declarations -fsanitize=address,undefined -g \
  -Isrc -Ivendor tests/library/test_parser.c $CORE_SOURCES $NET_SOURCES \
  src/library/json.c src/library/parser.c src/storage/book_file.c vendor/cJSON.c -lcurl -lm -o build/test_library
./build/test_library
cc -D_DARWIN_C_SOURCE -D_POSIX_C_SOURCE=200809L -std=c99 -Wall -Wextra -Wno-deprecated-declarations -fsanitize=address,undefined -g \
  -Isrc -Ivendor tests/library/test_download.c $CORE_SOURCES src/net/transfer.c \
  $LIBRARY_SOURCES vendor/cJSON.c -lcurl -lm -o build/test_download
./build/test_download
cc -D_DARWIN_C_SOURCE -D_POSIX_C_SOURCE=200809L -std=c99 -Wall -Wextra -fsanitize=address,undefined -g \
  -Isrc -Ivendor tests/storage/test_storage.c $CORE_SOURCES \
  src/storage/account.c src/storage/downloads.c vendor/cJSON.c -o build/test_storage
./build/test_storage
./scripts/test-ui.sh
./scripts/test-redirect.sh
