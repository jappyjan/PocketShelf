# PocketShelf

A standalone PocketBook application for browsing Z-Library, downloading books in PocketBook-supported formats, and opening them with PocketBook's built-in reader. No KOReader dependency.

**Authorized content only.** Use PocketShelf only for material that is lawfully available from the source and that you are entitled to download and, if enabled, upload to PocketBook Cloud. A working download link or library account is not evidence of permission. This independent project is not affiliated with Z-Library or PocketBook. See [copyright, dependency, and publication notes](docs/LEGAL.md).

**Status:** hardware testing on PocketBook Era Color. The native app launches and the login form accepts input. A live anonymous login probe now follows the site's same-origin cookie redirect and reaches its JSON login endpoint (`307 → 200`, then the expected invalid-credentials response for empty fields). Device diagnostics now confirm successful real-account sign-in and a 24-bit color canvas. The user has verified downloading and native book opening on the reader. Native Library registration is now requested after each successful download; its on-device visibility and Cloud upload still need verification. Parser, download-lifecycle, UI-state, and real local HTTPS redirect tests pass on macOS.

## Install

This source repository does not include a prebuilt binary. Run `./scripts/build.sh` with Docker installed to generate `dist/applications/`.

Connect the reader by USB in file-transfer mode. Merge the contents of `dist/applications/` into its `applications/` directory:

```
applications/
  PocketShelf.app
  pocketshelf/
    pocketshelf
```

Safely eject it, then launch **Apps → PocketShelf**. The small `.app` launcher starts the native ARM executable and records startup diagnostics. It does not change file associations or firmware.

1. If no session is saved, PocketShelf opens directly on its sign-in form.
2. On the sign-in screen, tap the Email and Password fields to edit each one using PocketBook's keyboard, then tap **Sign in**.
3. Successful sign-in opens the main menu, with your email at the top right. Use **Search the library** or **Browse popular books**.
4. Select an edition, read its **Description** and **Book info**, tap **Download [format]**, then **Open in PocketBook reader**.
5. Downloaded books remain available through **Downloaded books**, including when offline with a saved session.

Signing out returns to the login form. The main menu is hidden while logged out. There is no on-screen Close button on the main menu; use the physical Back button to exit from the main menu or login form. Inner pages have a Back button at the top left, while the signed-in identity stays at the top right.

Search defaults to all formats. Settings cycles through all formats supported by Era Color: EPUB, PDF, MOBI, AZW, AZW3, PRC, FB2, FB2.ZIP, DJVU, DOC, DOCX, RTF, TXT, HTM/HTML, CHM, CBR/CBZ, ACSM, plus its image and audio formats. DRM files still require the reader’s normal authorization. Physical next/previous buttons change result pages and description/info pages. Back returns to the previous screen. Network requests run in a worker thread, with a Cancel button.

Same-origin HTTPS redirects and their cookies are handled automatically using HTTP/1.1. If the site redirects to another origin or refuses API requests, enter a current HTTPS hostname you trust using the **Library address** field on the login form (or **Change library address** in Settings), then sign in again. The app does not bypass browser challenges. It has not implemented the website's Send-to-PocketBook action; direct native downloads are the implementation in this build.

Errors use red text and borders. The app requests a 24-bit color framebuffer for color PocketBook screens.

## Storage and troubleshooting

- Books: `Books/PocketShelf/` on the reader. After a completed download, PocketShelf calls the native BookPreparing/BookReady handoff on the InkView thread. Library indexing and Cloud upload are asynchronous. The first launch of this update also announces existing PocketShelf downloads, one at a time. **Account & settings → Refresh downloads in Library** repeats that repair without moving or duplicating files.
- Color covers: `system/config/pocketshelf/covers/`. Covers are cached and can be removed to reclaim space. Missing covers do not prevent browsing or downloads.
- Settings/session: `system/config/pocketshelf/account.json`. Passwords are not saved. A failed sign-in preserves the password in memory for retry; success or leaving the form clears it. Session tokens are stored locally and readable to anyone with USB access; **Sign out & forget session** removes the saved token.
- Startup and connection diagnostics (operation type, HTTP status, and curl result codes; no credentials or response bodies): `system/config/pocketshelf/last-run.log`. Reconnect via USB to retrieve this if the app does not start.
- TLS certificate verification remains enabled. If firmware trust fails, the error is shown; the app does not silently disable verification.
- API responses and cover images are capped at 4 MiB. Book downloads have no artificial size or total-time cap; storage and network availability still apply. Downloads use temporary files, check known container signatures, and are renamed only after successful completion. Repeated downloads receive distinct filenames.
- Cloud synchronization remains managed by PocketBook. Enable Auto upload and Autosync in the native PocketBook Cloud settings; this app neither changes those settings nor claims a completed upload.
- The local browser shows up to 256 files; other downloads remain accessible in the standard PocketBook library.
- The library API is unofficial and may change. Availability, account limits, and browser checks can prevent access. Use the website when it requires an interactive browser challenge.

## Build and tests

Requirements: Docker, Git, Python 3, OpenSSL, and a C compiler with libcurl for host tests. On macOS, Apple's command-line tools provide the test compiler and libcurl.

```sh
./scripts/test.sh
./scripts/build.sh
```

The build script clones the official SDK's `6.5` branch into `.cache/pocketbook-sdk` and runs its Linux x86-64 ARM compiler inside an Ubuntu container. It works on Apple Silicon through Docker's amd64 emulation. Output is in `dist/applications/`.

The tests exercise JSON shape changes, server errors, unsafe identifiers, credential/form escaping, cancellation callbacks, size limits, file signatures, repeated downloads, and cleanup after invalid/failed downloads. The redirect tests run against a temporary local HTTPS server with a test CA, preserving certificate verification. Tests also check origin isolation, cookie handshakes, redirect loops, login-only navigation, and error colors. They do not use a real account or assert hardware compatibility.

Source layout: `src/main.c` owns InkView UI and worker lifecycle; `src/library.c` owns protocol, parsing, HTTPS transport, and download storage. `vendor/` contains cJSON 1.7.19 under its MIT license. Protocol/source references and research are in [docs/research.md](docs/research.md).

To uninstall, remove `applications/PocketShelf.app` and `applications/pocketshelf/`. Remove `system/config/pocketshelf/` to clear settings. Downloaded books are independent and can be retained.

Book descriptions preserve paragraphs and decode HTML entities. Book info shows supplied bibliographic fields (including year/date, publisher, edition, volume, series, pages and ISBN), without inventing missing values. Four results per page leave room for readable cover thumbnails. A live saved-session metadata and cover request is tested without requesting a book download.

Overview metadata omits empty and whitespace-only values and inserts separators only between populated fields. Library registration is tested at successful/failed download completion, including UI-thread notification ordering; no PocketBook database is edited directly.
