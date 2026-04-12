/// @file database_demo.cpp
/// @brief Demonstrates SQLite RAII wrapper, prepared statements, transactions, and ORM.

#include <cstdio>
#include <string>

#include "database/sqlite.h"

// ── Model: User ─────────────────────────────────────────────────────
/// @brief Demo model for the database Repository pattern.
struct User {
    int64_t id = 0;
    std::string name;
    std::string email;
    int age = 0;

    static std::string table_name() { return "users"; }

    static std::string create_sql() {
        return "CREATE TABLE IF NOT EXISTS users ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "name TEXT NOT NULL,"
               "email TEXT NOT NULL UNIQUE,"
               "age INTEGER NOT NULL)";
    }

    static std::string insert_sql() {
        return "INSERT INTO users (name, email, age) VALUES (?, ?, ?)";
    }

    static std::string select_all_sql() { return "SELECT * FROM users"; }

    void bind_to(database::Statement& stmt) const {
        stmt.bind(1, name).bind(2, email).bind(3, age);
    }

    static User from_row(database::Statement& stmt) {
        return {stmt.col_int64(0), stmt.col_text(1),
                stmt.col_text(2), stmt.col_int(3)};
    }
};

int main() {
    using namespace database;

    // In-memory database for demo
    Database db(":memory:");
    Repository<User> users(db);

    // ── Insert via Repository ───────────────────────────────────────
    std::printf("=== Repository CRUD ===\n");
    users.insert({0, "Alice", "alice@example.com", 30});
    users.insert({0, "Bob", "bob@example.com", 25});

    for (auto& u : users.find_all())
        std::printf("  [%ld] %s <%s> age %d\n", static_cast<long>(u.id), u.name.c_str(),
                    u.email.c_str(), u.age);

    // ── Find by ID ──────────────────────────────────────────────────
    if (auto u = users.find_by_id(1))
        std::printf("  Found by id 1: %s\n", u->name.c_str());

    // ── Delete ──────────────────────────────────────────────────────
    users.remove(2);
    std::printf("  After deleting id 2: %zu users\n", users.find_all().size());

    // ── Transaction ─────────────────────────────────────────────────
    std::printf("\n=== Transaction ===\n");
    try {
        db.transaction([&] {
            users.insert({0, "Charlie", "charlie@example.com", 35});
            // Duplicate email triggers UNIQUE constraint → rollback
            users.insert({0, "Charlie2", "charlie@example.com", 40});
        });
    } catch (const SqliteError& e) {
        std::printf("  Rolled back: %s\n", e.what());
    }
    std::printf("  Users after failed txn: %zu\n", users.find_all().size());

    // ── Raw prepared statement ──────────────────────────────────────
    std::printf("\n=== Prepared Statement ===\n");
    auto stmt = db.prepare("SELECT name, age FROM users WHERE age >= ?");
    stmt.bind(1, 25);
    while (stmt.step())
        std::printf("  %s (age %d)\n", stmt.col_text(0).c_str(), stmt.col_int(1));

    return 0;
}
