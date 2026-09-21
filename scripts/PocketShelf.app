#!/bin/sh
# PocketBook application launcher. Keep its native executable alongside it.
APP_DIR=/mnt/ext1/applications/pocketshelf
STATE_DIR=/mnt/ext1/system/config/pocketshelf
umask 077
mkdir -p "$STATE_DIR"
exec "$APP_DIR/pocketshelf" >"$STATE_DIR/last-run.log" 2>&1
