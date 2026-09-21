#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build
. scripts/sources.sh
cc -D_DARWIN_C_SOURCE -std=c99 -Wall -Wextra -Wno-deprecated-declarations \
 -fsanitize=address,undefined -g -Isrc -Ivendor \
 tests/net/test_redirect.c $CORE_SOURCES $NET_SOURCES -lcurl -lm -o build/test_redirect
python3 tests/redirect_server.py
