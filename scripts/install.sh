#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
reader="${1:?Usage: scripts/install.sh /Volumes/PB700K3}"
[ -d "$reader/applications" ] && [ -d "$reader/system" ] || { echo 'This does not look like a mounted PocketBook.' >&2; exit 1; }
[ -f dist/applications/pocketshelf/pocketshelf ] || { echo 'Run scripts/build.sh first.' >&2; exit 1; }
export COPYFILE_DISABLE=1
mkdir -p "$reader/applications/pocketshelf"
cp dist/applications/pocketshelf/pocketshelf "$reader/applications/pocketshelf/pocketshelf.new"
cmp dist/applications/pocketshelf/pocketshelf "$reader/applications/pocketshelf/pocketshelf.new"
mv "$reader/applications/pocketshelf/pocketshelf.new" "$reader/applications/pocketshelf/pocketshelf"
cp dist/applications/PocketShelf.app "$reader/applications/PocketShelf.app.new"
cmp dist/applications/PocketShelf.app "$reader/applications/PocketShelf.app.new"
mv "$reader/applications/PocketShelf.app.new" "$reader/applications/PocketShelf.app"
printf 'Installed PocketShelf on %s. Safely eject before launching it.\n' "$reader"
