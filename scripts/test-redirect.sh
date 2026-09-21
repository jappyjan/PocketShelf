#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build
cc -D_DARWIN_C_SOURCE -std=c99 -Wall -Wextra -Wno-deprecated-declarations \
 -fsanitize=address,undefined -g -Isrc -Ivendor \
 tests/test_redirect.c vendor/cJSON.c -lcurl -lm -o build/test_redirect
python3 tests/redirect_server.py
