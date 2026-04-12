#pragma once

/// @file rest_server.h
/// @brief Modern C++ REST server built on cpp-httplib with JSON support.

#include <functional>
#include <string>
#include <utility>
#include <vector>

#ifdef HAS_JSON
#include <nlohmann/json.hpp>
#endif

#include <httplib.h>

namespace api {

#ifdef HAS_JSON
using json = nlohmann::json;
#endif

/// @brief HTTP status codes as a typed enum for clarity.
enum class Status : int {
    Ok = 200,           ///< 200 OK
    Created = 201,      ///< 201 Created
    NoContent = 204,    ///< 204 No Content
    BadRequest = 400,   ///< 400 Bad Request
    NotFound = 404,     ///< 404 Not Found
    InternalError = 500,///< 500 Internal Server Error
};

/// Thin wrapper providing a JSON-oriented REST API over httplib::Server.
///
/// Usage:
/// @code
///   api::RestServer server;
///   server.get("/items", [](auto& req, auto& res) {
///       res.set_json(Status::Ok, {{"items", json::array()}});
///   });
///   server.listen("0.0.0.0", 8080);
/// @endcode
class RestServer {
public:
    /// @brief Extended response helper wrapping httplib::Response.
    struct Response {
        httplib::Response& raw; ///< Underlying httplib response.

        /// @brief Set the HTTP status code.
        /// @param s Status code to set.
        void set_status(Status s) { raw.status = static_cast<int>(s); }

#ifdef HAS_JSON
        /// @brief Send a JSON response.
        /// @param s Status code to set.
        /// @param body JSON object to serialize as the response body.
        void set_json(Status s, const json& body) {
            raw.status = static_cast<int>(s);
            raw.set_content(body.dump(), "application/json");
        }
#endif

        /// @brief Send a plain-text response.
        /// @param s Status code to set.
        /// @param text Plain-text body content.
        void set_text(Status s, const std::string& text) {
            raw.status = static_cast<int>(s);
            raw.set_content(text, "text/plain");
        }
    };

    /// @brief Extended request helper wrapping httplib::Request.
    struct Request {
        const httplib::Request& raw; ///< Underlying httplib request.

        /// @brief Get a path parameter by key.
        /// @param key Parameter name.
        /// @return The parameter value, or empty string if not found.
        [[nodiscard]] std::string param(const std::string& key) const {
            auto it = raw.path_params.find(key);
            return it != raw.path_params.end() ? it->second : "";
        }

        /// @brief Get a query-string parameter by key.
        /// @param key Parameter name.
        /// @param def Default value if the parameter is absent.
        /// @return The parameter value, or @p def if not found.
        [[nodiscard]] std::string query(const std::string& key,
                                        const std::string& def = "") const {
            return raw.has_param(key) ? raw.get_param_value(key) : def;
        }

#ifdef HAS_JSON
        /// @brief Parse the request body as JSON.
        /// @return Parsed JSON object.
        /// @throws nlohmann::json::parse_error If the body is not valid JSON.
        [[nodiscard]] json body_json() const { return json::parse(raw.body); }
#endif
    };

    /// @brief Route handler signature.
    using Handler = std::function<void(const Request&, Response&)>;
    /// @brief Pre-handler middleware signature.
    using Middleware = std::function<void(const httplib::Request&,
                                         httplib::Response&)>;

    /// @brief Register pre-handler middleware (e.g., CORS, logging).
    /// @param mw Middleware function to invoke before each handler.
    void use(Middleware mw) { middleware_.push_back(std::move(mw)); }

    /// @brief Register a GET route.
    /// @param pattern URL pattern (may contain path parameters).
    /// @param h Handler to invoke on match.
    void get(const std::string& pattern, Handler h) { route("GET", pattern, std::move(h)); }
    /// @brief Register a POST route.
    /// @param pattern URL pattern.
    /// @param h Handler to invoke on match.
    void post(const std::string& pattern, Handler h) { route("POST", pattern, std::move(h)); }
    /// @brief Register a PUT route.
    /// @param pattern URL pattern.
    /// @param h Handler to invoke on match.
    void put(const std::string& pattern, Handler h) { route("PUT", pattern, std::move(h)); }
    /// @brief Register a DELETE route.
    /// @param pattern URL pattern.
    /// @param h Handler to invoke on match.
    void del(const std::string& pattern, Handler h) { route("DELETE", pattern, std::move(h)); }

    /// @brief Start listening (blocks the calling thread).
    /// @param host Bind address (e.g., "0.0.0.0").
    /// @param port TCP port number.
    /// @return `true` if the server started successfully.
    bool listen(const std::string& host, int port) {
        return server_.listen(host, port);
    }

    /// @brief Stop the server from another thread.
    void stop() { server_.stop(); }

    /// @brief Access underlying httplib::Server for advanced configuration.
    /// @return Reference to the internal httplib::Server.
    httplib::Server& raw() { return server_; }

private:
    void route(const char* method, const std::string& pattern, Handler h) {
        auto wrapped = [this, h = std::move(h)](const httplib::Request& req,
                                                 httplib::Response& res) {
            for (auto& mw : middleware_) mw(req, res);
            Request wreq{req};
            Response wres{res};
            try {
                h(wreq, wres);
            } catch (const std::exception& e) {
#ifdef HAS_JSON
                wres.set_json(Status::InternalError,
                              {{"error", e.what()}});
#else
                wres.set_text(Status::InternalError, e.what());
#endif
            }
        };

        if (std::string(method) == "GET") server_.Get(pattern, wrapped);
        else if (std::string(method) == "POST") server_.Post(pattern, wrapped);
        else if (std::string(method) == "PUT") server_.Put(pattern, wrapped);
        else if (std::string(method) == "DELETE") server_.Delete(pattern, wrapped);
    }

    httplib::Server server_;
    std::vector<Middleware> middleware_;
};

/// @brief CORS middleware factory.
/// @param origin Allowed origin header value (default: "*").
/// @return Middleware that sets CORS headers on every response.
inline RestServer::Middleware cors(const std::string& origin = "*") {
    return [origin](const httplib::Request& /*req*/, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", origin);
        res.set_header("Access-Control-Allow-Methods",
                       "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
    };
}

/// @brief Simple request-logging middleware factory.
/// @return Middleware that prints method and path to stdout.
inline RestServer::Middleware logger() {
    return [](const httplib::Request& req, httplib::Response& /*res*/) {
        std::printf("[%s] %s\n", req.method.c_str(), req.path.c_str());
    };
}

} // namespace api
