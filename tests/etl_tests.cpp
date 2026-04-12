/// @file etl_tests.cpp
/// @brief Unit tests for ETL pipeline (map, filter, take, reduce, count, flat_map).

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>
#include "etl/pipeline.h"

using namespace etl;

TEST_CASE("Pipeline map transforms elements", "[etl][pipeline]") {
    std::vector<int> data{1, 2, 3, 4};
    auto result = from(data).map([](int x) { return x * 2; }).collect();
    REQUIRE(result == std::vector<int>{2, 4, 6, 8});
}

TEST_CASE("Pipeline filter keeps matching elements", "[etl][pipeline]") {
    std::vector<int> data{1, 2, 3, 4, 5, 6};
    auto result = from(data).filter([](int x) { return x % 2 == 0; }).collect();
    REQUIRE(result == std::vector<int>{2, 4, 6});
}

TEST_CASE("Pipeline chained map + filter", "[etl][pipeline]") {
    std::vector<int> data{1, 2, 3, 4, 5};
    auto result = from(data)
        .map([](int x) { return x * x; })
        .filter([](int x) { return x > 5; })
        .collect();
    REQUIRE(result == std::vector<int>{9, 16, 25});
}

TEST_CASE("Pipeline take limits output", "[etl][pipeline]") {
    std::vector<int> data{10, 20, 30, 40, 50};
    auto result = from(data).take(3).collect();
    REQUIRE(result.size() == 3);
    REQUIRE(result == std::vector<int>{10, 20, 30});
}

TEST_CASE("Pipeline reduce folds elements", "[etl][pipeline]") {
    std::vector<int> data{1, 2, 3, 4};
    auto sum = from(data).reduce(0, std::plus<>{});
    REQUIRE(sum == 10);
}

TEST_CASE("Pipeline count returns element count", "[etl][pipeline]") {
    std::vector<int> data{1, 2, 3, 4, 5};
    auto n = from(data).filter([](int x) { return x > 3; }).count();
    REQUIRE(n == 2);
}

TEST_CASE("Pipeline on empty range", "[etl][pipeline]") {
    std::vector<int> data;
    REQUIRE(from(data).collect().empty());
    REQUIRE(from(data).count() == 0);
    REQUIRE(from(data).reduce(0, std::plus<>{}) == 0);
}

SCENARIO("ETL pipeline processes CSV-like data", "[etl][bdd]") {
    GIVEN("a vector of raw string records") {
        std::vector<std::string> records{"Alice,30", "Bob,25", "", "Carol,35"};

        WHEN("we filter blanks and extract names") {
            auto names = from(records)
                .filter([](const std::string& s) { return !s.empty(); })
                .map([](const std::string& s) { return s.substr(0, s.find(',')); })
                .collect();

            THEN("only non-empty names are returned") {
                REQUIRE(names.size() == 3);
                REQUIRE(names[0] == "Alice");
                REQUIRE(names[2] == "Carol");
            }
        }
    }
}
