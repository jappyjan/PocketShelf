#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
. scripts/sources.sh
mkdir -p build
cc -D_DARWIN_C_SOURCE -D_POSIX_C_SOURCE=200809L -std=c99 -Wall -Wextra -Wno-deprecated-declarations \
  -fsanitize=address,undefined -g -Itests/mock -Itests -Isrc -Ivendor \
  tests/app/*.c tests/support/*.c $CORE_SOURCES src/net/transfer.c \
  $APP_SOURCES $UI_SOURCES $PLATFORM_SOURCES vendor/cJSON.c -lcurl -lpthread -lm -o build/test_signin
./build/test_signin
