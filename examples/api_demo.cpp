/// @file api_demo.cpp
/// @brief Demonstrates REST server and client with a simple items CRUD API.

#include <api/rest_client.h>
#include <api/rest_server.h>

#include <cstdio>
#include <map>
#include <mutex>
#include <thread>

#ifdef HAS_JSON
using json = nlohmann::json;
#endif

// ── In-memory store ─────────────────────────────────────────────────
/// @brief In-memory item for the CRUD demo.
struct Item {
    int id;
    std::string name;
};

static std::mutex g_mu;
static std::map<int, Item> g_items;
static int g_next_id = 1;

// ── Server setup ────────────────────────────────────────────────────
/// @brief Register CRUD routes on the REST server.
void setup_routes(api::RestServer& srv) {
#ifdef HAS_JSON
    srv.use(api::cors());
    srv.use(api::logger());

    // List all items
    srv.get("/items", [](const auto& /*req*/, auto& res) {
        std::lock_guard lk(g_mu);
        auto arr = json::array();
        for (auto& [id, item] : g_items)
            arr.push_back({{"id", id}, {"name", item.name}});
        res.set_json(api::Status::Ok, {{"items", arr}});
    });

    // Get one item
    srv.get(R"(/items/(\d+))", [](const auto& req, auto& res) {
        int id = std::stoi(req.raw.matches[1]);
        std::lock_guard lk(g_mu);
        if (auto it = g_items.find(id); it != g_items.end()) {
            res.set_json(api::Status::Ok,
                         {{"id", it->second.id}, {"name", it->second.name}});
        } else {
            res.set_json(api::Status::NotFound, {{"error", "not found"}});
        }
    });

    // Create item
    srv.post("/items", [](const auto& req, auto& res) {
        auto body = req.body_json();
        std::lock_guard lk(g_mu);
        int id = g_next_id++;
        g_items[id] = {id, body.value("name", "unnamed")};
        res.set_json(api::Status::Created,
                     {{"id", id}, {"name", g_items[id].name}});
    });

    // Delete item
    srv.del(R"(/items/(\d+))", [](const auto& req, auto& res) {
        int id = std::stoi(req.raw.matches[1]);
        std::lock_guard lk(g_mu);
        if (g_items.erase(id))
            res.set_json(api::Status::Ok, {{"deleted", id}});
        else
            res.set_json(api::Status::NotFound, {{"error", "not found"}});
    });
#else
    srv.get("/", [](const auto&, auto& res) {
        res.set_text(api::Status::Ok, "Build with nlohmann-json for full demo");
    });
#endif
}

// ── Main ────────────────────────────────────────────────────────────
int main() {
    constexpr int port = 18080;

    api::RestServer server;
    setup_routes(server);

    // Run server in background thread
    std::thread srv_thread([&] { server.listen("127.0.0.1", port); });

    // Give server time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

#ifdef HAS_JSON
    // Exercise the API with the client
    api::RestClient client("127.0.0.1", port);

    std::puts("=== Creating items ===");
    auto r1 = client.post_json("/items", {{"name", "Widget"}});
    std::printf("  Created: %s\n", r1.body.dump().c_str());

    auto r2 = client.post_json("/items", {{"name", "Gadget"}});
    std::printf("  Created: %s\n", r2.body.dump().c_str());

    std::puts("\n=== Listing items ===");
    auto list = client.get_json("/items");
    std::printf("  %s\n", list.body.dump(2).c_str());

    std::puts("\n=== Get single item ===");
    auto single = client.get_json("/items/1");
    std::printf("  %s\n", single.body.dump().c_str());

    std::puts("\n=== Delete item 1 ===");
    auto del = client.del("/items/1");
    std::printf("  %s\n", del.body.c_str());

    std::puts("\n=== Final list ===");
    auto final_list = client.get_json("/items");
    std::printf("  %s\n", final_list.body.dump(2).c_str());
#else
    auto raw = api::RestClient("127.0.0.1", port).get("/");
    std::printf("Response: %s\n", raw.body.c_str());
#endif

    server.stop();
    srv_thread.join();
    std::puts("\nDone.");
}
