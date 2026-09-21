# Existing apps and native integration research

Checked 2026-09-21. User explicitly requires stock PocketBook reading, without KOReader.

- [ZlibraryKO/zlibrary.koplugin](https://github.com/ZlibraryKO/zlibrary.koplugin) provides the desired library features but requires KOReader, so it does not meet that requirement.
- [inkshelf](https://github.com/N-Combinator/inkshelf) and [Pocketbook-OPDSClient](https://github.com/j2robin/Pocketbook-OPDSClient) are native PocketBook catalog apps; their documented integrations are OPDS, not a verified Z-Library connection. No standalone stock-PocketBook Z-Library client was found in the searches. This is a search result, not proof that none exists.
- [PocketBook's official SDK, branch 6.5](https://github.com/pocketbook/SDK_6.3.0/tree/6.5) supplies the ARM compiler, sysroot and InkView API. The checked-out branch resolved to `dd8de463733a2bc0d53d1214b64734586fac427e`.
- [Official InkView header](https://github.com/pocketbook/SDK_6.3.0/blob/6.5/SDK-B288/usr/arm-obreey-linux-gnueabi/sysroot/usr/local/include/inkview.h) declares `OpenBook`, `NetConnect2`, `OpenKeyboard`, event handlers and timers. PocketShelf calls `OpenBook(path, "r", 0)` to hand a downloaded file to the stock reading system.
- [Native Era apps](https://github.com/Skrzypczy/pocketbook-apps) offer an additional native-build precedent. This is not evidence that PocketShelf has passed device tests.
- [Z-Library client protocol implementation](https://github.com/ZlibraryKO/zlibrary.koplugin/blob/main/zlibrary/api.lua) and [endpoint configuration](https://github.com/ZlibraryKO/zlibrary.koplugin/blob/main/zlibrary/config.lua) document the unofficial requests used here: form login through `/rpc.php`, JSON search through `/eapi/book/search`, popular books through `/eapi/book/most-popular`, and file links through `/eapi/book/{id}/{hash}/file`. This project implements those protocol interactions independently in C and does not run or distribute the KOReader plugin.
- [PocketBook Send-to-PocketBook documentation](https://sync.pocketbook-int.com/files/s2pb_info_en.pdf) describes delivery through a device email address. PocketShelf does not claim to implement Z-Library's send action or verify its current account requirements.

A real HTTPS probe to `z-library.sk/eapi/info/ok` from this Mac returned HTTP 517 and a DiamWall access-denied page. No real credentials were submitted and no account download was attempted. The app exposes a configurable base hostname and reports blocked/redirected requests instead of claiming success.

The USB volume identifies the connected device as `PB700K3` (Era Color). The ARM binary was compiled and inspected; hardware validation remains separate from compilation and fixture tests.

## On-device sign-in correction

The user confirmed the first build launches, but the password keyboard immediately
closed after completing email entry. The UI was opening `OpenKeyboard` for the
password from inside the email keyboard completion callback. The replacement
uses an explicit sign-in form, edits one field per user action, and defers redraw
until the keyboard callback has returned. A host-side InkView mock exercises the
real UI callbacks and rejects nested keyboard openings, redraws during completion,
and leaked pointer-up events. `scripts/test-signin.sh` failed against the original
flow and passes with the new form; on-device retesting is still required.

The next hardware attempt cleared the password without a useful visible result.
The host regression now exercises submission, an immediate rejected response,
and a successful retry. The form retains the password on failure, displays the
error on a dedicated screen, and clears the password on success or form exit.
The existing device log contained an `eth0` control-interface connection error,
which does not by itself establish the authentication failure cause. Added
diagnostics record only job type/outcome, native connection return code, HTTP
status, and curl result code; never credentials, request/response bodies, or URLs.

## Same-origin cookie redirect and color support

A later live anonymous probe established that `z-library.sk/rpc.php` returns a
307 redirect back to itself with a session cookie. Replaying the request with that
cookie over HTTP/1.1 returned HTTP 200 JSON. The app previously rejected all API
redirects. Its transport now accepts redirects only within the configured HTTPS
origin, retains an in-memory cookie jar, preserves POST on 307/308, discards
intermediate response bodies, and limits redirect hops. An actual libcurl probe
with empty credentials reached the login endpoint and received the expected
invalid-credentials message. No real credentials were used.

The default negotiated HTTP mode produced HTTP 517 after the first redirect in
the C probe; explicitly choosing HTTP/1.1 produced HTTP 200. The transport now
selects HTTP/1.1 for compatibility. A local HTTPS server verifies cookie transfer,
POST preservation, cross-origin rejection, HTTPS downgrade rejection and loop
limits without disabling TLS verification.

The official SDK header documents graphics colors as `0x00RRGGBB` and exports
`InitInkview`, `TASK_MAKEACTIVE`, and `TASK_FB_RGB24`. The app requests RGB24 before
`InkViewMain` and draws errors in `0x00D00000`. SDK binary inspection confirms
that `InkViewMain` does not reinitialize an already initialized screen. The
[native Qt integration example](https://github.com/fstanis/pocketbook-sdk-qt6#writing-an-app)
also initializes InkView before creating its screen. Actual color output remains
a hardware check; startup logs include canvas depth for diagnosis.

## Native library handoff (2026-09-21)

The connected device contained a completed EPUB in `Books/PocketShelf` but no matching folder/file in its `explorer-3` database (read-only inspection). The official SDK declares `BookPreparing` and `BookReady`. Its shipped InkView implementation sends a native task notification from BookReady. An independent native client also documents that the pair is required on some firmware: https://github.com/N-Combinator/inkshelf/blob/main/src/wifi_drop_ui.c. PocketShelf calls the SDK pair on the UI thread only after successful download completion, and re-announces earlier downloads on the first upgraded launch. No database writes or full-library resets are used.

PocketBook documents native Auto upload/Autosync at https://pocketbook.de/en/faq-ebooks-hoerbuchdownloads and synchronization triggers at https://cloud.pocketbook.digital/browser/en/faq/synchronization. Both boolean settings were enabled on the connected reader. Registration is a request; hardware indexing and eventual Cloud upload must still be verified after running the new build.
