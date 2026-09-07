# api — REST server & client (cpp-httplib + JSON)

Scope: `include/api/`. Namespace `api`. CMake target **`api` (INTERFACE / header-only)**.
Parent: [include/AGENTS.md](../AGENTS.md). Design notes: [docs/api_design.md](../../docs/api_design.md).

## Business role
Thin, JSON-oriented HTTP layer over **cpp-httplib**. Provides an ergonomic routing/response
facade so app code never touches raw httplib types. Used by `examples/api_demo.cpp`.

## ⚠️ Conditional compilation (read first)
- The **`api` target only exists when `cpp-httplib` is available** (`httplib_FOUND`).
  CMake looks for a package first (vcpkg/system `find_package(httplib)`), then **FetchContent**
  of pinned `v0.18.3` if that fails. If both fail, nothing here is built.
- **JSON is optional**, gated on `HAS_JSON` (nlohmann-json). CMake looks for a package first
  (`find_package(nlohmann_json)`), then **FetchContent** of pinned `v3.11.3` if that fails.
  Every JSON method (`set_json`, `body_json`, `get_json`, `post_json`, `put_json`, and the
  `json` alias) is inside `#ifdef HAS_JSON`. **The headers must still compile with `HAS_JSON`
  undefined** — keep the non-JSON `set_text` / raw `get`/`del` paths intact and add new JSON
  APIs under the guard.

## Files
- `rest_server.h` — `RestServer` (`get/post/put/del`, `use(middleware)`, `listen`, `stop`),
  wrapped `Request`/`Response` helpers, `Status` enum, and `cors()` / `logger()` middleware factories.
- `rest_client.h` — `RestClient` (`get_json`/`post_json`/`put_json`, raw `get`/`del`); throws
  `std::runtime_error` on transport failure.

## Invariants & business rules (MUST hold)
1. **Use the typed `Status` enum**, not bare ints, for response codes (`Status::Ok`,
   `NotFound`, `InternalError`, …). Extend the enum rather than sprinkling magic numbers.
2. **Handlers must not leak exceptions.** `RestServer::route` wraps every handler in a
   `try/catch`. JSON parse errors become **400** `{"error":"invalid JSON"}`. Any other
   `std::exception` becomes **500** with a generic `"internal error"` body — never `e.what()`.
   Keep this wrapper — it is the server's safety net.
3. **Middleware runs before the handler**, in registration order, for every matched route.
   If a middleware sets `res.status` to anything other than 200, remaining middleware and the
   handler are skipped (short-circuit). `cors()`/`logger()` do not change status. Middleware
   sees raw httplib types.
4. `Request::body_json()` calls `json::parse` and **throws on malformed input** — that throw
   is caught by the route wrapper and becomes **400**. Validate/parse inside the handler so
   the wrapper can turn failures into proper responses.
5. `RestClient` methods **throw on transport failure** (`check()` on a falsy `httplib::Result`).
   The client sets 5s connect / 30s read-write timeouts. Callers handle exceptions; success
   still returns the HTTP status for the caller to inspect.

## Security rules (this is a security-focused template)
- Treat all request input (`param`, `query`, body) as **untrusted**. Validate before use; if
  it reaches SQL, go through prepared statements in [database](../database/AGENTS.md) — never
  string-concatenate. If it reaches the shell/filesystem, sanitize.
- Set restrictive CORS in production (`cors("*")` is a permissive demo default — narrow the
  origin for real services).
- Don't log secrets or full request bodies in the `logger()` middleware.

## C++ best practices
- Keep everything `inline` (header-only). Pass `std::function` handlers by value + `std::move`.
- Prefer `std::string_view` for read-only string params where httplib's API allows.
- Access the underlying `httplib::Server`/`Client` via `raw()` only for genuinely advanced config.

## When editing
- Tests live in `tests/api_tests.cpp` (wired only when `httplib_FOUND`). Keep them on
  `127.0.0.1` with `bind_to_any_port` — do not listen on all interfaces in tests.
- New HTTP verb → add a `route(...)` dispatch branch **and** the public method.

## Neighbors
JSON types come from nlohmann-json (see [docs/third_party_integration.md](../../docs/third_party_integration.md)).
Persistence layer: [database](../database/AGENTS.md).
