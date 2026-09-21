# Architecture

PocketShelf is a native C application with one InkView event loop. Modules are
separated by responsibility; the core and library code can run without a reader
or UI. `main.c` creates the application state and starts the platform runtime.

## Where changes belong

| Folder | Owns | Does not own |
| --- | --- | --- |
| `core/` | Account/book records, supported formats, summary conversion, text helpers | HTTP, files, InkView |
| `net/` | HTTPS policy, redirects, TLS, bounded response sinks, cancellation/progress | JSON protocol, screens |
| `library/` | Endpoint requests, response parsing, cover/download transactions | Navigation or device events |
| `storage/` | Session persistence, local-book discovery, file signatures, device paths | UI state or network requests |
| `app/` | Navigation, actions, keyboard flow, foreground jobs, browsing/prefetch | Drawing primitives, HTTPS implementation |
| `ui/` | Screen rendering, reusable widgets, list layout and hitboxes | HTTP requests or persistence |
| `platform/` | InkView callback/timer adaptation, native Library handoff | Library protocol |

The library client calls the `http_request` interface. Production links the
libcurl adapter; download tests link a fixture adapter with the same interface.
UI tests similarly link fixture library operations and InkView functions. There
are no mutable transport hooks or test-time inclusions of implementation `.c`
files. The real HTTPS adapter is also tested against a local TLS server with an
explicit test CA; production uses the system trust store.

## State and lifetimes

`app/app.h` provides the application entry points. `app/state.h` is an internal
contract between application controllers and screen renderers, not a general
library interface. An `App` owns its account, selected book, view state, widgets,
local-book list and two independent worker snapshots. State is passed explicitly;
there are no process-wide UI state variables.

InkView callbacks do not take a context parameter. `platform/runtime.c` holds the
single active `App` pointer and translates those callbacks into context-taking
application functions. That is the only active-app binding. All navigation,
rendering, timers, file registration and view-state mutation run on the UI thread.

Each worker receives an immutable request snapshot in its own job. The worker
writes only its result and transfer progress. Completion is published under the
app mutex, and the UI thread joins the worker before consuming its result.
Cancellation and progress use the existing atomic transfer operations.

The browser owns cached summaries and its prefetch/cover worker. A new search
cancels the outstanding request and discards its eventual completion before
starting the replacement. Loading pages or covers never takes over the screen.
Foreground login/detail/download jobs retain their separate busy/cancel flow.
`EVT_EXIT` stops both workers, clears timers, releases browser records and fonts,
closes any repair scan, wipes sensitive fields, and destroys the mutex.

## Shared list module

`ui/list.c` takes a `ScrollList`: count, row height, offset, action base and a row
renderer/context. It owns clipping, row hitboxes, scroll bounds and page-step
geometry. It has no dependency on `App`, the catalog or networking.

`ui/catalog.c` adapts online and downloaded books to that module. Touch and
hardware-key events use the same geometry in `app/events.c`. Search and Popular
share the same online list. Search appends pages; Popular retains the complete
server collection unless pagination metadata explicitly says otherwise. Covers
load near the viewport without changing row dimensions or the scroll offset.

## Tests and builds

Run `./scripts/test.sh` for all sanitizer-enabled host tests, or
`./scripts/test-ui.sh` for the app regressions. Tests are grouped into `app/`,
`library/`, `net/` and `storage/`; shared fixtures live in `tests/support/`.
Existing regressions cover sign-in/navigation, keyboard teardown, native Library
handoff, detail text, infinite scrolling, Popular's 100-book response, stale
requests, download cleanup and HTTPS redirects. Storage tests exercise session
round trips, permissions and supported-file discovery in temporary directories.

`./scripts/build.sh` produces the ARM application with the official SDK. Host and
ARM builds use `scripts/sources.sh`; `scripts/check-architecture.py` verifies that
every production source is listed, checks lower-module dependency rules, and
rejects implementation-file includes. It runs as part of the test suite. `.clang-format` defines formatting; generated binaries, SDK
files and temporary diagnostics remain under ignored `build/`, `dist/`, `.cache/`.

The refactor intentionally retains screen layout, account file format, device
paths, endpoint behavior and download naming. Host tests do not substitute for
on-device touch and display verification.
