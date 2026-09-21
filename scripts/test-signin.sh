#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build
cc -D_DARWIN_C_SOURCE -std=c99 -Wall -Wextra -Wno-deprecated-declarations \
  -fsanitize=address,undefined -g -Itests/mock -Isrc -Ivendor \
  tests/test_signin.c src/library.c vendor/cJSON.c -lcurl -lpthread -lm -o build/test_signin
./build/test_signin
