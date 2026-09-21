# Shared source manifests for host tests and the PocketBook cross-build.
CORE_SOURCES="src/core/book.c src/core/text.c"
NET_SOURCES="src/net/transfer.c src/net/http.c"
LIBRARY_SOURCES="src/library/json.c src/library/parser.c src/library/client.c src/library/files.c src/storage/book_file.c"
APP_SOURCES="src/app/state.c src/app/actions.c src/app/events.c src/app/browser.c src/app/jobs.c src/app/keyboard.c"
UI_SOURCES="src/ui/widgets.c src/ui/list.c src/ui/catalog.c src/ui/render.c src/ui/detail.c src/ui/menu.c src/ui/auth.c"
PLATFORM_SOURCES="src/platform/runtime.c src/platform/library.c src/storage/account.c src/storage/downloads.c"
