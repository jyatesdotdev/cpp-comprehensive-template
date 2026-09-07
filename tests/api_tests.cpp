/// @file api_tests.cpp
/// @brief Unit tests for RestServer/RestClient (localhost, ephemeral port).

#include "api/rest_client.h"
#include "api/rest_server.h"

#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include <string>
#include <thread>

TEST_CASE("RestServer serves a text route", "[api]") {
    api::RestServer server;
    server.get("/ping", [](const auto &, auto &res) { res.set_text(api::Status::Ok, "pong"); });
    server.get("/boom", [](const auto &, auto &) { throw std::runtime_error("secret-details"); });

    const int port = server.raw().bind_to_any_port("127.0.0.1");
    REQUIRE(port > 0);
    std::thread t([&] { server.raw().listen_after_bind(); });

    api::RestClient client("127.0.0.1", port);
    auto ok = client.get("/ping");
    REQUIRE(ok.status == static_cast<int>(api::Status::Ok));
    REQUIRE(ok.body == "pong");

    auto err = client.get("/boom");
    REQUIRE(err.status == static_cast<int>(api::Status::InternalError));
    REQUIRE(err.body.find("secret-details") == std::string::npos);

    server.stop();
    t.join();
}

TEST_CASE("RestServer middleware non-200 status skips handler", "[api]") {
    api::RestServer server;
    bool handler_ran = false;
    server.use([](const httplib::Request &, httplib::Response &res) {
        res.status = 401;
        res.set_content("denied", "text/plain");
    });
    server.get("/secret", [&](const auto &, auto &res) {
        handler_ran = true;
        res.set_text(api::Status::Ok, "leaked");
    });

    const int port = server.raw().bind_to_any_port("127.0.0.1");
    REQUIRE(port > 0);
    std::thread t([&] { server.raw().listen_after_bind(); });

    api::RestClient client("127.0.0.1", port);
    auto r = client.get("/secret");
    REQUIRE(r.status == 401);
    REQUIRE(r.body == "denied");
    REQUIRE_FALSE(handler_ran);

    server.stop();
    t.join();
}

#ifdef HAS_JSON
TEST_CASE("RestServer maps invalid JSON to 400", "[api][json]") {
    api::RestServer server;
    server.post("/echo", [](const auto &req, auto &res) {
        auto body = req.body_json();
        res.set_json(api::Status::Ok, body);
    });

    const int port = server.raw().bind_to_any_port("127.0.0.1");
    REQUIRE(port > 0);
    std::thread t([&] { server.raw().listen_after_bind(); });

    api::RestClient client("127.0.0.1", port);
    auto res = client.raw().Post("/echo", "{", "application/json");
    REQUIRE(res);
    REQUIRE(res->status == static_cast<int>(api::Status::BadRequest));
    REQUIRE(res->body.find("invalid JSON") != std::string::npos);

    server.stop();
    t.join();
}
#endif
