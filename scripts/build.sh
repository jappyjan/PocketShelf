#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
SDK_DIR="${SDK_DIR:-$PWD/.cache/pocketbook-sdk}"
if [ ! -d "$SDK_DIR/SDK-B288" ]; then
  mkdir -p .cache
  git clone --depth 1 --branch 6.5 https://github.com/pocketbook/SDK_6.3.0.git "$SDK_DIR"
fi
mkdir -p build dist/applications/pocketshelf
docker run --rm --platform linux/amd64 \
  -v "$SDK_DIR/SDK-B288:/sdk:ro" -v "$PWD:/work" -w /work \
  ubuntu:22.04 sh -ec '
    . scripts/sources.sh
    SDK=/sdk/usr/arm-obreey-linux-gnueabi/sysroot
    export LD_LIBRARY_PATH=/sdk/usr/lib
    /sdk/usr/bin/arm-obreey-linux-gnueabi-gcc \
      --sysroot="$SDK" -D_GNU_SOURCE -std=gnu99 -O2 -Wall -Wextra \
      -I"$SDK/usr/local/include" -I"$SDK/usr/include/freetype2" -Isrc -Ivendor \
      src/main.c $CORE_SOURCES $NET_SOURCES $LIBRARY_SOURCES \
      $APP_SOURCES $UI_SOURCES $PLATFORM_SOURCES vendor/cJSON.c \
      -L"$SDK/usr/local/lib" -Wl,-rpath-link,"$SDK/usr/lib" \
      -o dist/applications/pocketshelf/pocketshelf -linkview -lcurl -lpthread -lm
    /sdk/usr/bin/arm-obreey-linux-gnueabi-strip dist/applications/pocketshelf/pocketshelf
    /sdk/usr/bin/arm-obreey-linux-gnueabi-readelf -h dist/applications/pocketshelf/pocketshelf | head -20
  '

cp scripts/PocketShelf.app dist/applications/PocketShelf.app
chmod +x dist/applications/PocketShelf.app dist/applications/pocketshelf/pocketshelf
