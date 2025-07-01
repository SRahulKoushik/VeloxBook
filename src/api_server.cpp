// Drogon HTTP/WebSocket API server for OrderBook
#include <drogon/drogon.h>
#include <json/json.h>
#include "matching_engine.hpp"
#include "order.hpp"
#include "utils.hpp"
#include "OrderBookController.h"
#include "OrderBookWebSocket.h"
#include <memory>
#include <iostream>
#include <fstream>
#include <chrono>
#include <atomic>
#include <thread>
#include <drogon/utils/Utilities.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpFilter.h>
#include "jwt-cpp/jwt.h"
#include <signal.h>

using namespace orderbook;

// Production API key - use environment variable
const std::string get_api_key() {
    const char* env_key = std::getenv("ORDERBOOK_API_KEY");
    if (env_key) {
        return std::string(env_key);
    }
    return "my-secret-key"; // Fallback for development
}

// Production log file path
const std::string get_log_file() {
    const char* env_log = std::getenv("ORDERBOOK_LOG_FILE");
    if (env_log) {
        return std::string(env_log);
    }
    return "orderbook.log"; // Fallback for development
}

// Global for metrics
std::atomic<size_t> g_order_count{0};
std::atomic<size_t> g_trade_count{0};
std::atomic<double> g_last_order_latency_ms{0.0};

// Rate limiting map
std::unordered_map<std::string, std::chrono::steady_clock::time_point> rate_limit_map;
std::mutex rate_limit_mutex;

// Simple string sanitizer
std::string sanitize(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (isalnum(c) || c == '_' || c == '-') out += c;
    }
    return out;
}

// Rate limiting filter
class RateLimitFilter : public drogon::HttpFilter<RateLimitFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr &req, drogon::FilterCallback &&fcb, drogon::FilterChainCallback &&fccb) override {
        auto client_ip = req->getPeerAddr().toIp();
        auto now = std::chrono::steady_clock::now();
        
        std::lock_guard<std::mutex> lock(rate_limit_mutex);
        auto it = rate_limit_map.find(client_ip);
        
        if (it != rate_limit_map.end()) {
            auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);
            if (time_diff.count() < 100) { // 10 requests per second
                Json::Value errJson;
                errJson["error"] = "Rate limit exceeded";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(errJson);
                resp->setStatusCode(drogon::k429TooManyRequests);
                fcb(resp);
                return;
            }
        }
        
        rate_limit_map[client_ip] = now;
        fccb();
    }
};

// Append JSON to log file
void append_log(const Json::Value& entry) {
    std::ofstream ofs(get_log_file(), std::ios::app);
    Json::StreamWriterBuilder wbuilder;
    wbuilder["indentation"] = "  ";
    std::unique_ptr<Json::StreamWriter> writer(wbuilder.newStreamWriter());
    writer->write(entry, &ofs);
    ofs << std::endl;
}

// JWT Auth Filter
class JwtAuthFilter : public drogon::HttpFilter<JwtAuthFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr &req, drogon::FilterCallback &&fcb, drogon::FilterChainCallback &&fccb) override {
        try {
            auto authHeader = req->getHeader("authorization");
            if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
                throw std::runtime_error("Missing or invalid Authorization header");
            }
            std::string token = authHeader.substr(7);
            auto decoded = jwt::decode(token);
            auto verifier = jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{orderbook::get_jwt_secret()})
                .with_issuer("orderbook");
            verifier.verify(decoded);

            Json::Value jwtClaims;
            auto payload = decoded.get_payload_json();
            for (const auto& pair : payload) {
                if (pair.second.is<std::string>())
                    jwtClaims[pair.first] = pair.second.get<std::string>();
                else
                    jwtClaims[pair.first] = pair.second.serialize();
            }
            req->attributes()->insert("jwt", jwtClaims);
            fccb();
        } catch (const std::exception &e) {
            Json::Value errJson;
            errJson["error"] = "Unauthorized: invalid or missing JWT";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(errJson);
            resp->setStatusCode(drogon::k401Unauthorized);
            fcb(resp);
        }
    }
};

int main() {
    std::cout << "== ORDERBOOK SERVER STARTING ==" << std::endl;
    std::cout << "Drogon version: " << drogon::getVersion() << std::endl;

    // Set up signal handlers for graceful shutdown
    signal(SIGINT, [](int) {
        std::cout << "\n[SERVER] Shutting down..." << std::endl;
        drogon::app().quit();
    });
    signal(SIGTERM, [](int) {
        std::cout << "\n[SERVER] Received SIGTERM, shutting down..." << std::endl;
        drogon::app().quit();
    });

    // Enable CORS for frontend integration (always for dev, safe for prod if needed)
    // drogon::app().enableCors();

    // Serve frontend static files in production (uncomment and set correct path)
    // drogon::app().setDocumentRoot("../../frontend/dist");
    // If your frontend build output is in a different folder, adjust the path above.

    // Hardcoded DB client
    using namespace drogon::orm;
    auto dbClient = DbClient::newPgClient("host=127.0.0.1 port=5432 dbname=orderbookdb user=rahulorderbook password=SRK2905boss?!", 5);
    if (!dbClient) {
        std::cerr << "[DB FATAL] Please check:" << std::endl;
        std::cerr << "  - PostgreSQL server is running" << std::endl;
        std::cerr << "  - Database credentials are correct" << std::endl;
        std::cerr << "  - Network connectivity" << std::endl;
        return 1;
    }

    // Create required tables synchronously to ensure they exist before querying
    try {
        std::cout << "[DB] Creating database tables..." << std::endl;
        
        // Create orders table
        dbClient->execSqlSync(
            "CREATE TABLE IF NOT EXISTS orders (id TEXT PRIMARY KEY, symbol TEXT, side TEXT, type TEXT, price BIGINT, quantity BIGINT, user_id TEXT, status TEXT);"
        );
        std::cout << "[DB] Orders table ready" << std::endl;
        
        // Create actions table
        dbClient->execSqlSync(
            "CREATE TABLE IF NOT EXISTS actions (action TEXT, order_id TEXT, price BIGINT, quantity BIGINT, ts TIMESTAMPTZ DEFAULT NOW());"
        );
        std::cout << "[DB] Actions table ready" << std::endl;
        
        // Create trades table
        dbClient->execSqlSync(
            "CREATE TABLE IF NOT EXISTS trades (symbol TEXT, buy_order_id TEXT, sell_order_id TEXT, price BIGINT, quantity BIGINT, ts TIMESTAMPTZ DEFAULT NOW());"
        );
        std::cout << "[DB] Trades table ready" << std::endl;
        
        std::cout << "[DB] All tables created successfully" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "[DB ERROR] Failed to create tables: " << e.what() << std::endl;
        return 1;
    }

    MatchingEngine engine;

    // --- Replay DB state ---
    try {
        std::cout << "[DB] Replaying database state..." << std::endl;
        
        auto actionsResult = dbClient->execSqlSync("SELECT action, order_id, price, quantity FROM actions ORDER BY ts ASC;");
        std::cout << "[DB] Found " << actionsResult.size() << " actions to replay" << std::endl;
        
        for (const auto &row : actionsResult) {
            std::string action = row[0].as<std::string>();
            std::string order_id = row[1].as<std::string>();
            Price price = row[2].as<Price>();
            Quantity quantity = row[3].as<Quantity>();
            if (action == "cancel") engine.cancel_order(order_id);
            else if (action == "modify") engine.modify_order(order_id, price, quantity);
        }

        auto openOrders = dbClient->execSqlSync("SELECT id, symbol, side, type, price, quantity, user_id FROM orders WHERE status='open' OR status='partial';");
        std::cout << "[DB] Found " << openOrders.size() << " open orders to restore" << std::endl;
        
        for (const auto &row : openOrders) {
            auto order = std::make_shared<Order>(
                row[0].as<std::string>(),
                row[1].as<std::string>(),
                row[2].as<std::string>() == "buy" ? OrderSide::BUY : OrderSide::SELL,
                row[3].as<std::string>() == "market" ? OrderType::MARKET :
                    (row[3].as<std::string>() == "limit" ? OrderType::LIMIT :
                    (row[3].as<std::string>() == "stop" ? OrderType::STOP : OrderType::STOP_LIMIT)),
                row[4].as<Price>(),
                row[5].as<Quantity>(),
                row[6].as<std::string>()
            );
            engine.add_order(order);
        }

        auto tradesResult = dbClient->execSqlSync("SELECT symbol, buy_order_id, sell_order_id, price, quantity, ts FROM trades ORDER BY ts ASC;");
        std::cout << "[DB] Found " << tradesResult.size() << " trades to restore" << std::endl;
        
        for (const auto &row : tradesResult) {
            Trade trade;
            trade.symbol = row[0].as<std::string>();
            trade.buy_order_id = row[1].as<std::string>();
            trade.sell_order_id = row[2].as<std::string>();
            trade.price = row[3].as<Price>();
            trade.quantity = row[4].as<Quantity>();
            engine.add_trade_history(trade);
        }
        
        std::cout << "[DB] Database state replay completed successfully" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "[DB] Replay failed: " << e.what() << " — continuing with fresh state" << std::endl;
    }

    // Attach engine to controller
    OrderBookController::setEngine(&engine);
    OrderBookController::setDbClient(dbClient);
    OrderBookController::setMetrics(&g_order_count, &g_trade_count, &g_last_order_latency_ms);

    // Create and register WebSocket controller
    auto wsController = std::make_shared<OrderBookWebSocket>(&engine);
    OrderBookController::setWebSocketController(wsController.get());

    // Register controllers with Drogon
    std::cout << "[ROUTES] Registering HTTP controller..." << std::endl;
    drogon::app().registerController(std::make_shared<OrderBookController>());
    std::cout << "[ROUTES] Registering WebSocket controller..." << std::endl;
    drogon::app().registerController(wsController);
    std::cout << "[ROUTES] Controllers registered successfully" << std::endl;

    // Add a simple test route to verify routing is working
    drogon::app().registerHandler("/test", [](const drogon::HttpRequestPtr& req, 
                                              std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        Json::Value response;
        response["message"] = "Test endpoint working!";
        response["timestamp"] = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        callback(resp);
    }, {drogon::Get});
    std::cout << "[ROUTES] Test route /test registered" << std::endl;

    // Start background expiry thread
    std::thread([](MatchingEngine *eng) {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            eng->cancel_expired_orders();
        }
    }, &engine).detach();

    // Start server
    drogon::app().addListener("0.0.0.0", 18080);
    std::cout << "[SERVER] Drogon running at http://localhost:18080\n";
    drogon::app().run();
    return 0;
}

