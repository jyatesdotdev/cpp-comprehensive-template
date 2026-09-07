/// @file database_tests.cpp
/// @brief Unit tests for the SQLite RAII wrapper and Repository template.

#include "database/sqlite.h"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string>
#include <string_view>

using namespace database;

namespace {

struct User {
    int64_t id = 0;
    std::string name;
    int age = 0;

    static std::string table_name() { return "users"; }
    static std::string create_sql() {
        return "CREATE TABLE IF NOT EXISTS users ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "name TEXT NOT NULL,"
               "age INTEGER NOT NULL)";
    }
    static std::string insert_sql() { return "INSERT INTO users (name, age) VALUES (?, ?)"; }
    static std::string select_all_sql() { return "SELECT id, name, age FROM users"; }

    void bind_to(Statement &stmt) const { stmt.bind(1, name).bind(2, age); }
    static User from_row(Statement &stmt) {
        return {stmt.col_int64(0), stmt.col_text(1), stmt.col_int(2)};
    }
};

} // namespace

TEST_CASE("Database exec/prepare/bind", "[database]") {
    Database db(":memory:");
    db.exec("CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT)");
    auto stmt = db.prepare("INSERT INTO t (name) VALUES (?)");
    stmt.bind(1, std::string_view{"alice"});
    REQUIRE_FALSE(stmt.step());
    REQUIRE(db.last_insert_rowid() == 1);

    auto sel = db.prepare("SELECT name FROM t WHERE id = ?");
    sel.bind(1, 1);
    REQUIRE(sel.step());
    REQUIRE(sel.col_text(0) == "alice");
    REQUIRE_FALSE(sel.step());
}

TEST_CASE("Repository CRUD and transaction rollback", "[database]") {
    Database db(":memory:");
    Repository<User> users(db);
    users.insert({0, "Ada", 36});
    users.insert({0, "Bob", 25});
    REQUIRE(users.find_all().size() == 2);
    auto ada = users.find_by_id(1);
    REQUIRE(ada.has_value());
    REQUIRE(ada->name == "Ada");

    users.remove(2);
    REQUIRE(users.find_all().size() == 1);

    REQUIRE_THROWS_AS(db.transaction([&] {
        users.insert({0, "Cara", 40});
        throw SqliteError(1, "forced");
    }),
                      SqliteError);
    REQUIRE(users.find_all().size() == 1);
}

TEST_CASE("Repository rejects unsafe table_name identifiers", "[database]") {
    struct Evil {
        int64_t id = 0;
        static std::string table_name() { return "users; DROP TABLE users;--"; }
        static std::string create_sql() {
            return "CREATE TABLE IF NOT EXISTS dummy (id INTEGER PRIMARY KEY)";
        }
        static std::string insert_sql() { return "INSERT INTO dummy (id) VALUES (?)"; }
        static std::string select_all_sql() { return "SELECT id FROM dummy"; }
        void bind_to(Statement &stmt) const { stmt.bind(1, id); }
        static Evil from_row(Statement &stmt) { return {stmt.col_int64(0)}; }
    };

    Database db(":memory:");
    Repository<Evil> repo(db);
    REQUIRE_THROWS_AS(repo.find_by_id(1), SqliteError);
    REQUIRE_THROWS_AS(repo.remove(1), SqliteError);
}
