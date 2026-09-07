# database — SQLite RAII wrapper & Repository ORM

Scope: `include/database/`. Namespace `database`. CMake target **`database` (INTERFACE /
header-only)**. Parent: [include/AGENTS.md](../AGENTS.md).

## Business role
A safe, modern C++ face over the SQLite3 C API: RAII connections and statements, typed
bind/column accessors, transactions, and a tiny Repository (ORM) template. Used by
`examples/database_demo.cpp`.

## ⚠️ Conditional compilation
The **`database` target only exists when SQLite3 is found** (`SQLite3_FOUND`). All code
includes `<sqlite3.h>` directly.

## Files
- `sqlite.h` — `SqliteError` (carries the SQLite result code), `Statement` (RAII prepared
  statement), `Database` (RAII connection), and `Repository<T>` (CRUD for a model type).

## Invariants & business rules (MUST hold — several are security rules)
1. **SQL injection: always use bound parameters.** Build queries with `?` placeholders and
   `Statement::bind(idx, value)` — **never concatenate untrusted values into SQL.** Text binds
   use `SQLITE_TRANSIENT` (SQLite copies the buffer), so binding a `string_view` is safe even
   if the source is temporary.
   - Note: `Repository::find_by_id/remove` concatenate `T::table_name()` into the SQL. The *id*
     is bound, and `table_name()` is rejected unless it matches `[A-Za-z_][A-Za-z0-9_]*`
     (`SqliteError` otherwise). Keep any interpolated identifier whitelisted; bind every value.
2. **Index bases differ and are a classic bug source:** `bind(...)` is **1-based**;
   `col_*(...)` reads are **0-based**. Preserve this and document it at call sites.
3. **RAII ownership is exclusive.** `Database` and `Statement` are move-only (copy deleted);
   moves null out the source handle; destructors call `sqlite3_close_v2` / `sqlite3_finalize`.
   Never copy them, never double-close, never store a raw `sqlite3*`/`sqlite3_stmt*` that
   outlives its wrapper.
4. **Errors are exceptions.** Non-`OK`/unexpected result codes throw `SqliteError`. `step()`
   returns `true` on `SQLITE_ROW`, `false` on `SQLITE_DONE`, and throws otherwise. Don't
   swallow return codes silently.
5. **`transaction(fn)` is BEGIN → fn → COMMIT, with ROLLBACK on any exception**, then rethrow.
   ROLLBACK uses `sqlite3_exec` directly so a failed rollback cannot hide the original error.
   Use it for multi-statement atomicity; don't hand-roll BEGIN/COMMIT.
6. On open, the connection sets `PRAGMA journal_mode=WAL` and `PRAGMA foreign_keys=ON`. Keep
   these (WAL concurrency + enforced FKs are intended defaults).
7. **`Repository<T>` is a static-interface contract.** `T` must provide `table_name()`,
   `create_sql()`, `insert_sql()`, `select_all_sql()`, `bind_to(Statement&) const`, and
   `static T from_row(Statement&)`. Document these on any model you add; a missing one is a
   compile error (by design).

## C++ best practices
- Keep bind/column overloads type-safe; add new SQLite types as overloads, not casts.
- `[[nodiscard]]` on `prepare`, column getters, `last_insert_rowid`, `find_*`.
- Reuse a prepared `Statement` across rows with `reset()` (it also clears bindings) rather
  than re-preparing in a loop.
- SQLite handles are **not** thread-safe across connections by default — one `Database` per
  thread unless you know the threading mode.

## When editing
- Tests live in `tests/database_tests.cpp` (wired only when `SQLite3_FOUND`). Prefer
  `:memory:` databases so tests stay offline and isolated.

## Neighbors
Frequently paired with [api](../api/AGENTS.md) (validate HTTP input → bind into SQL).
Repository is one of the design patterns catalogued in [docs/ARCHITECTURE.md](../../docs/ARCHITECTURE.md).
