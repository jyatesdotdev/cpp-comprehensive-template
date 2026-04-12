#pragma once

/// @file rest_client.h
/// @brief Typed HTTP client wrapper around cpp-httplib.

#include <optional>
#include <stdexcept>
#include <string>

#ifdef HAS_JSON
#include <nlohmann/json.hpp>
#endif

#include <httplib.h>

namespace api {

#ifdef HAS_JSON
using json = nlohmann::json;
#endif

/// Lightweight HTTP client with JSON helpers.
///
/// Usage:
/// @code
///   api::RestClient client("localhost", 8080);
///   auto [status, body] = client.get_json("/items");
/// @endcode
class RestClient {
public:
    /// @brief Construct a client connected to the given host and port.
    /// @param host Server hostname or IP address.
    /// @param port Server TCP port.
    RestClient(const std::string& host, int port)
        : client_(host, port) {}

    /// @brief Plain HTTP response (status code + body string).
    struct RawResponse {
        int status;        ///< HTTP status code.
        std::string body;  ///< Response body as a string.
    };

#ifdef HAS_JSON
    /// @brief JSON HTTP response (status code + parsed JSON body).
    struct JsonResponse {
        int status;  ///< HTTP status code.
        json body;   ///< Parsed JSON response body.
    };

    /// @brief Perform a GET request and parse the response as JSON.
    /// @param path Request path (e.g., "/items").
    /// @return JsonResponse with status and parsed body.
    /// @throws std::runtime_error If the request fails.
    JsonResponse get_json(const std::string& path) {
        auto res = client_.Get(path);
        check(res);
        return {res->status, json::parse(res->body)};
    }

    /// @brief Perform a POST request with a JSON body.
    /// @param path Request path.
    /// @param data JSON object to send as the request body.
    /// @return JsonResponse with status and parsed body.
    /// @throws std::runtime_error If the request fails.
    JsonResponse post_json(const std::string& path, const json& data) {
        auto res = client_.Post(path, data.dump(), "application/json");
        check(res);
        return {res->status, res->body.empty() ? json{} : json::parse(res->body)};
    }

    /// @brief Perform a PUT request with a JSON body.
    /// @param path Request path.
    /// @param data JSON object to send as the request body.
    /// @return JsonResponse with status and parsed body.
    /// @throws std::runtime_error If the request fails.
    JsonResponse put_json(const std::string& path, const json& data) {
        auto res = client_.Put(path, data.dump(), "application/json");
        check(res);
        return {res->status, res->body.empty() ? json{} : json::parse(res->body)};
    }
#endif

    /// @brief Perform a raw GET request.
    /// @param path Request path.
    /// @return RawResponse with status and body string.
    /// @throws std::runtime_error If the request fails.
    RawResponse get(const std::string& path) {
        auto res = client_.Get(path);
        check(res);
        return {res->status, res->body};
    }

    /// @brief Perform a raw DELETE request.
    /// @param path Request path.
    /// @return RawResponse with status and body string.
    /// @throws std::runtime_error If the request fails.
    RawResponse del(const std::string& path) {
        auto res = client_.Delete(path);
        check(res);
        return {res->status, res->body};
    }

    /// @brief Access underlying httplib::Client for advanced configuration.
    /// @return Reference to the internal httplib::Client.
    httplib::Client& raw() { return client_; }

private:
    static void check(const httplib::Result& res) {
        if (!res) {
            throw std::runtime_error("HTTP request failed: " +
                                     httplib::to_string(res.error()));
        }
    }

    httplib::Client client_;
};

} // namespace api
