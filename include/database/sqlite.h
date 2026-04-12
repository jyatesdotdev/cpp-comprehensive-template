#pragma once

/// @file sqlite.h
/// @brief Modern C++ RAII wrapper for SQLite3 with prepared statements and ORM patterns.

#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <sqlite3.h>

namespace database {

/// @brief Exception type for SQLite errors.
class SqliteError : public std::runtime_error {
public:
    /// @brief Construct an error with an SQLite result code and message.
    /// @param code SQLite error code (e.g. SQLITE_ERROR).
    /// @param msg Human-readable error description.
    SqliteError(int code, const std::string& msg)
        : std::runtime_error(msg + " (code " + std::to_string(code) + ")"),
          code_(code) {}
    /// @brief Get the SQLite error code.
    /// @return The SQLite result code that triggered this error.
    [[nodiscard]] int code() const noexcept { return code_; }

private:
    int code_;
};

class Database; // forward

/// @brief RAII prepared statement with typed bind/column accessors.
class Statement {
public:
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    Statement(Statement&& o) noexcept : stmt_(o.stmt_) { o.stmt_ = nullptr; }
    Statement& operator=(Statement&& o) noexcept {
        if (this != &o) { finalize(); stmt_ = o.stmt_; o.stmt_ = nullptr; }
        return *this;
    }
    ~Statement() { finalize(); }

    // ── Bind (1-based index) ────────────────────────────────────────
    /// @brief Bind an integer value to a parameter.
    /// @param idx 1-based parameter index.
    /// @param value Integer value to bind.
    /// @return Reference to this statement for chaining.
    /// @throws SqliteError on bind failure.
    Statement& bind(int idx, int value) {
        check(sqlite3_bind_int(stmt_, idx, value));
        return *this;
    }
    /// @brief Bind a 64-bit integer value to a parameter.
    /// @param idx 1-based parameter index.
    /// @param value 64-bit integer value to bind.
    /// @return Reference to this statement for chaining.
    /// @throws SqliteError on bind failure.
    Statement& bind(int idx, int64_t value) {
        check(sqlite3_bind_int64(stmt_, idx, value));
        return *this;
    }
    /// @brief Bind a double value to a parameter.
    /// @param idx 1-based parameter index.
    /// @param value Double value to bind.
    /// @return Reference to this statement for chaining.
    /// @throws SqliteError on bind failure.
    Statement& bind(int idx, double value) {
        check(sqlite3_bind_double(stmt_, idx, value));
        return *this;
    }
    /// @brief Bind a text value to a parameter.
    /// @param idx 1-based parameter index.
    /// @param value String view to bind (copied via SQLITE_TRANSIENT).
    /// @return Reference to this statement for chaining.
    /// @throws SqliteError on bind failure.
    Statement& bind(int idx, std::string_view value) {
        check(sqlite3_bind_text(stmt_, idx, value.data(),
                                static_cast<int>(value.size()), SQLITE_TRANSIENT));
        return *this;
    }
    /// @brief Bind NULL to a parameter.
    /// @param idx 1-based parameter index.
    /// @return Reference to this statement for chaining.
    /// @throws SqliteError on bind failure.
    Statement& bind_null(int idx) {
        check(sqlite3_bind_null(stmt_, idx));
        return *this;
    }

    // ── Step / Reset ────────────────────────────────────────────────
    /// @brief Execute the next step of the statement.
    /// @return true if a row is available (SQLITE_ROW), false on SQLITE_DONE.
    /// @throws SqliteError on any other result code.
    bool step() {
        int rc = sqlite3_step(stmt_);
        if (rc == SQLITE_ROW) return true;
        if (rc == SQLITE_DONE) return false;
        throw SqliteError(rc, sqlite3_errmsg(sqlite3_db_handle(stmt_)));
    }

    /// @brief Reset the statement for re-execution and clear bindings.
    /// @return Reference to this statement for chaining.
    /// @throws SqliteError on reset failure.
    Statement& reset() {
        check(sqlite3_reset(stmt_));
        sqlite3_clear_bindings(stmt_);
        return *this;
    }

    // ── Column access (0-based) ─────────────────────────────────────
    /// @brief Get an integer column value.
    /// @param idx 0-based column index.
    /// @return The column value as int.
    [[nodiscard]] int col_int(int idx) const { return sqlite3_column_int(stmt_, idx); }
    /// @brief Get a 64-bit integer column value.
    /// @param idx 0-based column index.
    /// @return The column value as int64_t.
    [[nodiscard]] int64_t col_int64(int idx) const { return sqlite3_column_int64(stmt_, idx); }
    /// @brief Get a double column value.
    /// @param idx 0-based column index.
    /// @return The column value as double.
    [[nodiscard]] double col_double(int idx) const { return sqlite3_column_double(stmt_, idx); }
    /// @brief Get a text column value.
    /// @param idx 0-based column index.
    /// @return The column value as std::string (empty if NULL).
    [[nodiscard]] std::string col_text(int idx) const {
        auto* p = reinterpret_cast<const char*>(sqlite3_column_text(stmt_, idx));
        return p ? std::string(p) : std::string{};
    }
    /// @brief Check whether a column value is NULL.
    /// @param idx 0-based column index.
    /// @return true if the column is NULL.
    [[nodiscard]] bool col_is_null(int idx) const {
        return sqlite3_column_type(stmt_, idx) == SQLITE_NULL;
    }
    /// @brief Get the number of columns in the result set.
    /// @return Column count.
    [[nodiscard]] int column_count() const { return sqlite3_column_count(stmt_); }

private:
    friend class Database;
    explicit Statement(sqlite3_stmt* s) : stmt_(s) {}

    void check(int rc) {
        if (rc != SQLITE_OK)
            throw SqliteError(rc, sqlite3_errmsg(sqlite3_db_handle(stmt_)));
    }
    void finalize() { if (stmt_) sqlite3_finalize(stmt_); stmt_ = nullptr; }

    sqlite3_stmt* stmt_ = nullptr;
};

/// @brief RAII SQLite database connection.
class Database {
public:
    /// @brief Open a database connection at the given path.
    /// @param path Filesystem path to the SQLite database file.
    /// @throws SqliteError if the database cannot be opened.
    explicit Database(const std::string& path) {
        int rc = sqlite3_open(path.c_str(), &db_);
        if (rc != SQLITE_OK) {
            std::string msg = db_ ? sqlite3_errmsg(db_) : "cannot open database";
            sqlite3_close(db_);
            throw SqliteError(rc, msg);
        }
        exec("PRAGMA journal_mode=WAL");
        exec("PRAGMA foreign_keys=ON");
    }

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    Database(Database&& o) noexcept : db_(o.db_) { o.db_ = nullptr; }
    Database& operator=(Database&& o) noexcept {
        if (this != &o) { close(); db_ = o.db_; o.db_ = nullptr; }
        return *this;
    }
    ~Database() { close(); }

    /// @brief Prepare a SQL statement.
    /// @param sql SQL query string with optional `?` placeholders.
    /// @return A prepared Statement ready for binding and execution.
    /// @throws SqliteError if preparation fails.
    [[nodiscard]] Statement prepare(std::string_view sql) {
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(db_, sql.data(),
                                    static_cast<int>(sql.size()), &stmt, nullptr);
        if (rc != SQLITE_OK)
            throw SqliteError(rc, sqlite3_errmsg(db_));
        return Statement(stmt);
    }

    /// @brief Execute SQL that returns no rows.
    /// @param sql SQL statement to execute.
    /// @throws SqliteError on execution failure.
    void exec(std::string_view sql) {
        char* err = nullptr;
        int rc = sqlite3_exec(db_, std::string(sql).c_str(), nullptr, nullptr, &err);
        if (rc != SQLITE_OK) {
            std::string msg = err ? err : "exec error";
            sqlite3_free(err);
            throw SqliteError(rc, msg);
        }
    }

    /// @brief Get the rowid of the last successful INSERT.
    /// @return The rowid as int64_t.
    [[nodiscard]] int64_t last_insert_rowid() const {
        return sqlite3_last_insert_rowid(db_);
    }

    /// @brief Run a callable inside a BEGIN/COMMIT transaction; rolls back on exception.
    /// @tparam F Callable type (invoked with no arguments).
    /// @param fn The function to execute within the transaction.
    /// @return The return value of @p fn (if non-void).
    /// @throws Rethrows any exception from @p fn after issuing ROLLBACK.
    template <typename F>
    auto transaction(F&& fn) -> decltype(fn()) {
        exec("BEGIN");
        try {
            if constexpr (std::is_void_v<decltype(fn())>) {
                fn();
                exec("COMMIT");
            } else {
                auto result = fn();
                exec("COMMIT");
                return result;
            }
        } catch (...) {
            exec("ROLLBACK");
            throw;
        }
    }

private:
    void close() { if (db_) sqlite3_close(db_); db_ = nullptr; }
    sqlite3* db_ = nullptr;
};

// ── Simple ORM Pattern ──────────────────────────────────────────────

/// @brief A lightweight Repository template providing CRUD for a model type.
///
/// The model @p T must provide:
///   - `static std::string table_name();`
///   - `static std::string create_sql();`       — CREATE TABLE statement
///   - `static std::string insert_sql();`       — INSERT with ? placeholders
///   - `static std::string select_all_sql();`   — SELECT *
///   - `void bind_to(Statement& stmt) const;`   — bind fields to insert stmt
///   - `static T from_row(Statement& stmt);`    — construct from SELECT row
/// @tparam T Model type satisfying the above interface.
template <typename T>
class Repository {
public:
    /// @brief Construct a repository and ensure the table exists.
    /// @param db Database connection to use.
    explicit Repository(Database& db) : db_(db) {
        db_.exec(T::create_sql());
    }

    /// @brief Insert an entity into the table.
    /// @param entity The model instance to insert.
    void insert(const T& entity) {
        auto stmt = db_.prepare(T::insert_sql());
        entity.bind_to(stmt);
        stmt.step();
    }

    /// @brief Retrieve all entities from the table.
    /// @return A vector of all model instances.
    [[nodiscard]] std::vector<T> find_all() {
        auto stmt = db_.prepare(T::select_all_sql());
        std::vector<T> results;
        while (stmt.step()) results.push_back(T::from_row(stmt));
        return results;
    }

    /// @brief Find an entity by its primary key.
    /// @param id The rowid to look up.
    /// @return The entity if found, or std::nullopt.
    [[nodiscard]] std::optional<T> find_by_id(int64_t id) {
        auto stmt = db_.prepare(
            std::string("SELECT * FROM ") + T::table_name() + " WHERE id = ?");
        stmt.bind(1, id);
        if (stmt.step()) return T::from_row(stmt);
        return std::nullopt;
    }

    /// @brief Delete an entity by its primary key.
    /// @param id The rowid to delete.
    void remove(int64_t id) {
        auto stmt = db_.prepare(
            std::string("DELETE FROM ") + T::table_name() + " WHERE id = ?");
        stmt.bind(1, id);
        stmt.step();
    }

private:
    Database& db_;
};

} // namespace database
