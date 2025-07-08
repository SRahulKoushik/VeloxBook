#include "OrderBookController.h"
#include "OrderBookWebSocket.h"
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <algorithm>
#include <chrono>
#include <atomic>
#include <sstream>
#include <drogon/utils/Utilities.h>
#include "utils.hpp"
#include "jwt-cpp/jwt.h"
#include <future>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

// Global pointers to the core system components
// I keep these static so all controller instances can access the same engine
MatchingEngine* OrderBookController::engine = nullptr;
std::atomic<size_t>* OrderBookController::g_order_count = nullptr;
std::atomic<size_t>* OrderBookController::g_trade_count = nullptr;
std::atomic<double>* OrderBookController::g_last_order_latency_ms = nullptr;
OrderBookWebSocket* OrderBookController::wsController = nullptr;
drogon::orm::DbClientPtr OrderBookController::dbClient = nullptr;

// Demo variables for the async/concurrency demo endpoint
// These show how to use std::mutex, std::condition_variable, etc.
std::mutex demo_mutex;
std::condition_variable demo_cv;
bool demo_ready = false;
int demo_shared_value = 0;

void OrderBookController::setEngine(MatchingEngine* eng) {
    engine = eng;
}
void OrderBookController::setMetrics(std::atomic<size_t>* oc, std::atomic<size_t>* tc, std::atomic<double>* lat) {
    g_order_count = oc; g_trade_count = tc; g_last_order_latency_ms = lat;
}
void OrderBookController::setWebSocketController(OrderBookWebSocket* ws) { wsController = ws; }
void OrderBookController::setDbClient(drogon::orm::DbClientPtr dbClient_) {
    dbClient = dbClient_;
}

// Clean up user input to prevent injection attacks
// Only allows alphanumeric chars, underscores, and hyphens
static std::string sanitize(const std::string& s) {
    std::string out;
    for (char c : s) if (isalnum(c) || c == '_' || c == '-') out += c;
    return out;
}

// Validate that the order JSON has all required fields
// Returns false and sets error message if validation fails
static bool validate_order_json(const Json::Value& body, std::string& err) {
    if (!body.isMember("symbol") || !body["symbol"].isString()) { err = "Missing or invalid 'symbol'"; return false; }
    if (!body.isMember("side") || !body["side"].isString()) { err = "Missing or invalid 'side'"; return false; }
    if (!body.isMember("type") || !body["type"].isString()) { err = "Missing or invalid 'type'"; return false; }
    if (!body.isMember("price") || !body["price"].isUInt64()) { err = "Missing or invalid 'price'"; return false; }
    if (!body.isMember("quantity") || !body["quantity"].isUInt64()) { err = "Missing or invalid 'quantity'"; return false; }
    if (!body.isMember("user_id") || !body["user_id"].isString()) { err = "Missing or invalid 'user_id'"; return false; }
    if (body.isMember("expiry") && !body["expiry"].isUInt64()) { err = "Invalid 'expiry'"; return false; }
    if (body.isMember("tif") && !body["tif"].isString()) { err = "Invalid 'tif'"; return false; }
    return true;
}

// Add CORS headers so the frontend can talk to the backend
// Without this, browsers block cross-origin requests
void add_cors_headers(const drogon::HttpResponsePtr& resp) {
    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    resp->addHeader("Access-Control-Allow-Credentials", "true");
}

void OrderBookController::placeOrder(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    auto t0 = std::chrono::high_resolution_clock::now();
    try {
        Json::Value body;
        Json::CharReaderBuilder builder;
       	std::string errs;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        std::string bodyStr = std::string(req->getBody());
        if (!reader->parse(bodyStr.c_str(), bodyStr.c_str() + bodyStr.size(), &body, &errs)) {
            Json::Value errJson;
            errJson["error"] = "Invalid JSON";
            auto resp = HttpResponse::newHttpJsonResponse(errJson);
            resp->setStatusCode(k400BadRequest);
            add_cors_headers(resp);
            callback(resp);
            return;
        }
        std::string err;
        if (!validate_order_json(body, err)) {
            Json::Value errJson;
            errJson["error"] = err;
            auto resp = HttpResponse::newHttpJsonResponse(errJson);
            resp->setStatusCode(k400BadRequest);
            add_cors_headers(resp);
            callback(resp);
            return;
        }
        std::string symbol = sanitize(body["symbol"].asString());
        std::string user_id = sanitize(body["user_id"].asString());
        std::string side = sanitize(body["side"].asString());
        std::string type = sanitize(body["type"].asString());
        int64_t expiry = 0;
        std::string tif = "GTC";
        if (body.isMember("expiry")) expiry = body["expiry"].asInt64();
        if (body.isMember("tif")) tif = sanitize(body["tif"].asString());
        auto order = std::make_shared<Order>(
            std::to_string(now_nanoseconds()),
            symbol,
            side == "buy" ? OrderSide::BUY : OrderSide::SELL,
            type == "market" ? OrderType::MARKET :
                (type == "limit" ? OrderType::LIMIT :
                (type == "stop" ? OrderType::STOP : OrderType::STOP_LIMIT)),
            static_cast<Price>(body["price"].asUInt64()),
            static_cast<Quantity>(body["quantity"].asUInt64()),
            user_id,
            body.isMember("stop_price") ? static_cast<Price>(body["stop_price"].asUInt64()) : 0,
            expiry,
            tif
        );
        auto trades = engine->add_order(order);
        // Handle Time-in-Force orders (IOC = Immediate or Cancel, FOK = Fill or Kill)
        if (tif == "IOC" && order->filled_quantity < order->quantity) engine->cancel_order(order->id);
        if (tif == "FOK" && order->filled_quantity < order->quantity) engine->cancel_order(order->id);
        if (g_order_count) (*g_order_count)++;
        if (g_trade_count) (*g_trade_count) += trades.size();
        auto t1 = std::chrono::high_resolution_clock::now();
        if (g_last_order_latency_ms) *g_last_order_latency_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        // Save the order to the database asynchronously
        if (dbClient) {
            dbClient->execSqlAsync(
                "INSERT INTO orders (id, symbol, side, type, price, quantity, user_id, status) VALUES ($1,$2,$3,$4,$5,$6,$7,$8) ON CONFLICT (id) DO UPDATE SET symbol=EXCLUDED.symbol, side=EXCLUDED.side, type=EXCLUDED.type, price=EXCLUDED.price, quantity=EXCLUDED.quantity, user_id=EXCLUDED.user_id, status=EXCLUDED.status;",
                [](const drogon::orm::Result& result) { /* Success */ },
                [](const std::exception_ptr& e) { /* Error */ },
                std::string(order->id), std::string(symbol), std::string(side), std::string(type), order->price, order->quantity, std::string(user_id),
                order->status == OrderStatus::FILLED ? std::string("filled") :
                order->status == OrderStatus::PARTIAL ? std::string("partial") :
                order->status == OrderStatus::REJECTED ? std::string("rejected") : std::string("open")
            );
            dbClient->execSqlAsync(
                "INSERT INTO actions (action, order_id, price, quantity) VALUES ($1,$2,$3,$4);",
                [](const drogon::orm::Result& result) { /* Success */ },
                [](const std::exception_ptr& e) { /* Error */ },
                std::string("add"), std::string(order->id), order->price, order->quantity
            );
        }
        // Send real-time updates via WebSocket
        if (wsController) {
            wsController->broadcastOrderBook(symbol);
            for (const auto& trade : trades) {
                Json::Value tradeMsg;
                tradeMsg["type"] = "trade";
                tradeMsg["symbol"] = trade.symbol;
                tradeMsg["buy_order_id"] = trade.buy_order_id;
                tradeMsg["sell_order_id"] = trade.sell_order_id;
                tradeMsg["price"] = trade.price;
                tradeMsg["quantity"] = trade.quantity;
                Json::StreamWriterBuilder wbuilder;
                std::string payload = Json::writeString(wbuilder, tradeMsg);
                wsController->broadcastTrade(symbol, payload);
            }
        }
        // Build the response with order status and any trades that happened
        Json::Value resj;
        resj["status"] = order->status == OrderStatus::FILLED ? "filled" :
                          order->status == OrderStatus::PARTIAL ? "partial" :
                          order->status == OrderStatus::REJECTED ? "rejected" : "open";
        resj["order_id"] = order->id;
        resj["trades"] = Json::Value(Json::arrayValue);
        for (const auto& trade : trades) {
            Json::Value t;
            t["buy_order_id"] = trade.buy_order_id;
            t["sell_order_id"] = trade.sell_order_id;
            t["price"] = trade.price;
            t["quantity"] = trade.quantity;
            t["symbol"] = trade.symbol;
            resj["trades"].append(t);
        }
        auto resp = HttpResponse::newHttpJsonResponse(resj);
        add_cors_headers(resp);
        callback(resp);
    } catch (const std::exception& e) {
        Json::Value errJson;
        errJson["error"] = "Invalid request format";
        auto resp = HttpResponse::newHttpJsonResponse(errJson);
        resp->setStatusCode(k400BadRequest);
        add_cors_headers(resp);
        callback(resp);
    }
}

void OrderBookController::cancelOrder(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string orderId) {
    orderId = sanitize(orderId);
    bool success = engine->cancel_order(orderId);
    // Log the cancellation in the database
    if (dbClient && success) {
        dbClient->execSqlAsync(
            "INSERT INTO actions (action, order_id) VALUES ($1,$2);",
            [](const drogon::orm::Result& result) { /* Success */ },
            [](const std::exception_ptr& e) { /* Error */ },
            std::string("cancel"), std::string(orderId)
        );
    }
    if (!success) {
        Json::Value errJson;
        errJson["error"] = "Order not found or already filled/cancelled";
        auto resp = HttpResponse::newHttpJsonResponse(errJson);
        resp->setStatusCode(k404NotFound);
        add_cors_headers(resp);
        callback(resp);
        return;
    }
    
    // Broadcast order book updates after successful cancellation
    if (wsController) {
        // Get all symbols that might have been affected
        auto all_orders = engine->get_all_orders();
        std::set<std::string> affected_symbols;
        for (const auto& order : all_orders) {
            affected_symbols.insert(order->symbol);
        }
        // Broadcast updates for all symbols (or implement symbol lookup for cancelled order)
        for (const auto& symbol : affected_symbols) {
            wsController->broadcastOrderBook(symbol);
        }
    }
    
    Json::Value resJson;
    resJson["result"] = "Cancelled";
    auto resp = HttpResponse::newHttpJsonResponse(resJson);
    add_cors_headers(resp);
    callback(resp);
}

void OrderBookController::modifyOrder(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    try {
        Json::Value body;
        Json::CharReaderBuilder builder;
        std::string errs;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        std::string bodyStr = std::string(req->getBody());
        if (!reader->parse(bodyStr.c_str(), bodyStr.c_str() + bodyStr.size(), &body, &errs)) {
            Json::Value errJson;
            errJson["error"] = "Invalid JSON";
            auto resp = HttpResponse::newHttpJsonResponse(errJson);
            resp->setStatusCode(k400BadRequest);
            add_cors_headers(resp);
            callback(resp);
            return;
        }
        if (!body.isMember("order_id") || !body["order_id"].isString() ||
            !body.isMember("price") || !body["price"].isUInt64() ||
            !body.isMember("quantity") || !body["quantity"].isUInt64()) {
            Json::Value errJson;
            errJson["error"] = "Missing or invalid fields";
            auto resp = HttpResponse::newHttpJsonResponse(errJson);
            resp->setStatusCode(k400BadRequest);
            add_cors_headers(resp);
            callback(resp);
            return;
        }
        std::string order_id = sanitize(body["order_id"].asString());
        bool success = engine->modify_order(
            order_id,
            static_cast<Price>(body["price"].asUInt64()),
            static_cast<Quantity>(body["quantity"].asUInt64())
        );
        // Insert modify action into PostgreSQL
        if (dbClient && success) {
            dbClient->execSqlAsync(
                "INSERT INTO actions (action, order_id, price, quantity) VALUES ($1,$2,$3,$4);",
                [](const drogon::orm::Result& result) { /* Success */ },
                [](const std::exception_ptr& e) { /* Error */ },
                std::string("modify"), std::string(order_id), static_cast<Price>(body["price"].asUInt64()), static_cast<Quantity>(body["quantity"].asUInt64())
            );
        }
        // --- WebSocket broadcast ---
        if (success && wsController) {
            // Get all symbols that might have been affected
            auto all_orders = engine->get_all_orders();
            std::set<std::string> affected_symbols;
            for (const auto& order : all_orders) {
                affected_symbols.insert(order->symbol);
            }
            // Broadcast updates for all symbols (or implement symbol lookup for modified order)
            for (const auto& symbol : affected_symbols) {
                wsController->broadcastOrderBook(symbol);
            }
        }
        if (!success) {
            auto resp = HttpResponse::newHttpJsonResponse(Json::Value({{"error", "Order not found or not modifiable"}}));
            resp->setStatusCode(k404NotFound);
            add_cors_headers(resp);
            callback(resp);
            return;
        }
        auto resp = HttpResponse::newHttpJsonResponse(Json::Value({{"result", "Modified"}}));
        add_cors_headers(resp);
        callback(resp);
    } catch (...) {
        auto resp = HttpResponse::newHttpJsonResponse(Json::Value({{"error", "Invalid request format"}}));
        resp->setStatusCode(k400BadRequest);
        add_cors_headers(resp);
        callback(resp);
    }
}

void OrderBookController::getOrderById(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, std::string orderId) {
    orderId = sanitize(orderId);
    auto user_orders = engine->get_all_orders(); // You may need to implement this in your engine
    for (const auto& order : user_orders) {
        if (order->id == orderId) {
            std::string type_str = order->type == OrderType::MARKET ? "market" :
                                   order->type == OrderType::LIMIT ? "limit" :
                                   order->type == OrderType::STOP ? "stop" : "stop_limit";
            std::string status_str = order->status == OrderStatus::FILLED ? "filled" :
                                     order->status == OrderStatus::PARTIAL ? "partial" :
                                     order->status == OrderStatus::CANCELLED ? "cancelled" :
                                     order->status == OrderStatus::REJECTED ? "rejected" : "open";
            auto ts = std::chrono::duration_cast<std::chrono::seconds>(order->timestamp.time_since_epoch()).count();
            Json::Value resj;
            resj["id"] = order->id;
            resj["symbol"] = order->symbol;
            resj["side"] = order->side == OrderSide::BUY ? "buy" : "sell";
            resj["type"] = type_str;
            resj["price"] = order->price;
            resj["quantity"] = order->quantity;
            resj["filled"] = order->filled_quantity;
            resj["status"] = status_str;
            resj["timestamp"] = ts;
            resj["expiry"] = order->expiry;
            resj["tif"] = order->tif;
            auto resp = HttpResponse::newHttpJsonResponse(resj);
            add_cors_headers(resp);
            callback(resp);
            return;
        }
    }
    auto resp = HttpResponse::newHttpJsonResponse(Json::Value({{"error", "Order not found"}}));
    resp->setStatusCode(k404NotFound);
    add_cors_headers(resp);
    callback(resp);
}

void OrderBookController::getTradeHistory(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, std::string userId) {
    userId = sanitize(userId);
    // You may need to implement engine->get_user_trades(userId)
    auto trades = engine->get_user_trades(userId);
    Json::Value resj = Json::arrayValue;
    for (const auto& trade : trades) {
        Json::Value t;
        t["symbol"] = trade.symbol;
        t["buy_order_id"] = trade.buy_order_id;
        t["sell_order_id"] = trade.sell_order_id;
        t["price"] = trade.price;
        t["quantity"] = trade.quantity;
        // Convert time_point to timestamp (milliseconds since epoch)
        auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(trade.timestamp.time_since_epoch()).count();
        t["timestamp"] = timestamp_ms;
        resj.append(t);
    }
    auto resp = HttpResponse::newHttpJsonResponse(resj);
    add_cors_headers(resp);
    callback(resp);
}

void OrderBookController::getOrders(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string userId) {
    userId = sanitize(userId);
    int page = 1, page_size = 50;
    std::string status_filter, symbol_filter;
    int64_t from_ts = 0, to_ts = 0;
    bool history = false;
    auto q = req->getParameters();
    if (q.find("page") != q.end()) page = std::max(1, std::stoi(q["page"]));
    if (q.find("page_size") != q.end()) page_size = std::max(1, std::min(500, std::stoi(q["page_size"])));
    if (q.find("status") != q.end()) status_filter = sanitize(q["status"]);
    if (q.find("symbol") != q.end()) symbol_filter = sanitize(q["symbol"]);
    if (q.find("from_ts") != q.end()) from_ts = std::stoll(q["from_ts"]);
    if (q.find("to_ts") != q.end()) to_ts = std::stoll(q["to_ts"]);
    if (q.find("history") != q.end() && (q["history"] == "1" || q["history"] == "true")) history = true;
    std::vector<std::shared_ptr<Order>> user_orders = engine->get_user_orders(userId);
    std::vector<std::shared_ptr<Order>> filtered;
    for (const auto& order : user_orders) {
        std::string status_str = order->status == OrderStatus::FILLED ? "filled" :
                                 order->status == OrderStatus::PARTIAL ? "partial" :
                                 order->status == OrderStatus::CANCELLED ? "cancelled" :
                                 order->status == OrderStatus::REJECTED ? "rejected" : "open";
        if (!status_filter.empty() && status_str != status_filter) continue;
        if (!symbol_filter.empty() && order->symbol != symbol_filter) continue;
        auto ts = std::chrono::duration_cast<std::chrono::seconds>(order->timestamp.time_since_epoch()).count();
        if (from_ts > 0 && ts < from_ts) continue;
        if (to_ts > 0 && ts > to_ts) continue;
        if (history) {
            if (status_str != "filled" && status_str != "cancelled") continue;
        } else {
            if (status_str == "filled" || status_str == "cancelled") continue;
        }
        filtered.push_back(order);
    }
    int total = filtered.size();
    int start = (page - 1) * page_size;
    int end = std::min(start + page_size, total);
    Json::Value resj = Json::arrayValue;
    for (int i = start; i < end; ++i) {
        const auto& order = filtered[i];
        std::string type_str = order->type == OrderType::MARKET ? "market" :
                               order->type == OrderType::LIMIT ? "limit" :
                               order->type == OrderType::STOP ? "stop" : "stop_limit";
        std::string status_str = order->status == OrderStatus::FILLED ? "filled" :
                                 order->status == OrderStatus::PARTIAL ? "partial" :
                                 order->status == OrderStatus::CANCELLED ? "cancelled" :
                                 order->status == OrderStatus::REJECTED ? "rejected" : "open";
        auto ts = std::chrono::duration_cast<std::chrono::seconds>(order->timestamp.time_since_epoch()).count();
        Json::Value o;
        o["id"] = order->id;
        o["symbol"] = order->symbol;
        o["side"] = order->side == OrderSide::BUY ? "buy" : "sell";
        o["type"] = type_str;
        o["price"] = order->price;
        o["quantity"] = order->quantity;
        o["filled"] = order->filled_quantity;
        o["status"] = status_str;
        o["timestamp"] = ts;
        o["expiry"] = order->expiry;
        o["tif"] = order->tif;
        resj.append(o);
    }
    Json::Value out;
    out["orders"] = resj;
    out["page"] = page;
    out["page_size"] = page_size;
    out["total"] = total;
    auto resp = HttpResponse::newHttpJsonResponse(out);
    add_cors_headers(resp);
    callback(resp);
}

void OrderBookController::getOrderBook(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string symbol) {
    symbol = sanitize(symbol);
    int page = 1, page_size = 50;
    auto q = req->getParameters();
    if (q.find("page") != q.end()) page = std::max(1, std::stoi(q["page"]));
    if (q.find("page_size") != q.end()) page_size = std::max(1, std::min(500, std::stoi(q["page_size"])));
    Json::Value resj;
    auto bids = engine->get_bid_levels(symbol, 1000);
    auto asks = engine->get_ask_levels(symbol, 1000);
    int bid_total = bids.size(), ask_total = asks.size();
    int bid_start = (page - 1) * page_size, bid_end = std::min(bid_start + page_size, bid_total);
    int ask_start = (page - 1) * page_size, ask_end = std::min(ask_start + page_size, ask_total);
    resj["bids"] = Json::arrayValue;
    resj["asks"] = Json::arrayValue;
    for (int i = bid_start; i < bid_end; ++i) {
        Json::Value bid;
        bid["price"] = bids[i].price;
        bid["quantity"] = bids[i].total_quantity;
        resj["bids"].append(bid);
    }
    for (int i = ask_start; i < ask_end; ++i) {
        Json::Value ask;
        ask["price"] = asks[i].price;
        ask["quantity"] = asks[i].total_quantity;
        resj["asks"].append(ask);
    }
    resj["page"] = page;
    resj["page_size"] = page_size;
    resj["bid_total"] = bid_total;
    resj["ask_total"] = ask_total;
    auto resp = HttpResponse::newHttpJsonResponse(resj);
    add_cors_headers(resp);
    callback(resp);
}

void OrderBookController::health(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    Json::Value resj;
    resj["status"] = "ok";
    resj["db"] = (dbClient != nullptr);
    resj["engine_alive"] = true;
    auto resp = HttpResponse::newHttpJsonResponse(resj);
    add_cors_headers(resp);
    callback(resp);
}

void OrderBookController::metrics(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    std::ostringstream oss;
    oss << "# HELP orderbook_orders_total Total number of orders received\n";
    oss << "# TYPE orderbook_orders_total counter\n";
    oss << "orderbook_orders_total " << (g_order_count ? g_order_count->load() : 0) << "\n";
    oss << "# HELP orderbook_trades_total Total number of trades executed\n";
    oss << "# TYPE orderbook_trades_total counter\n";
    oss << "orderbook_trades_total " << (g_trade_count ? g_trade_count->load() : 0) << "\n";
    oss << "# HELP orderbook_last_order_latency Last order processing latency in milliseconds\n";
    oss << "# TYPE orderbook_last_order_latency gauge\n";
    oss << "orderbook_last_order_latency " << (g_last_order_latency_ms ? g_last_order_latency_ms->load() : 0.0) << "\n";
    auto resp = HttpResponse::newHttpResponse();
    resp->setContentTypeString("text/plain; version=0.0.4");
    resp->setBody(oss.str());
    add_cors_headers(resp);
    callback(resp);
}

void OrderBookController::registerUser(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto json = req->getJsonObject();
    if (!json || !(*json).isMember("username") || !(*json).isMember("password")) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value({{"error", "Missing username or password"}}));
        resp->setStatusCode(drogon::k400BadRequest);
        add_cors_headers(resp);
        callback(resp);
        return;
    }
    std::string username = (*json)["username"].asString();
    std::string password = (*json)["password"].asString();

    // Username: 3-20 chars, alphanumeric or underscore
    if (username.length() < 3 || username.length() > 20 ||
        !std::all_of(username.begin(), username.end(), [](char c) { return std::isalnum(c) || c == '_'; })) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value({{"error", "Username must be 3-20 characters, only letters, numbers, and underscores allowed."}}));
        resp->setStatusCode(drogon::k400BadRequest);
        add_cors_headers(resp);
        callback(resp);
        return;
    }
    // Password: 6-64 chars, at least one uppercase, one lowercase, one digit, one special
    if (password.length() < 6 || password.length() > 64) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value({{"error", "Password must be 6-64 characters."}}));
        resp->setStatusCode(drogon::k400BadRequest);
        add_cors_headers(resp);
        callback(resp);
        return;
    }
    bool has_upper = false, has_lower = false, has_digit = false, has_special = false;
    for (char c : password) {
        if (std::isupper(static_cast<unsigned char>(c))) has_upper = true;
        else if (std::islower(static_cast<unsigned char>(c))) has_lower = true;
        else if (std::isdigit(static_cast<unsigned char>(c))) has_digit = true;
        else if (std::ispunct(static_cast<unsigned char>(c))) has_special = true;
    }
    if (!has_upper || !has_lower || !has_digit || !has_special) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value({{"error", "Password must contain at least one uppercase letter, one lowercase letter, one digit, and one special character."}}));
        resp->setStatusCode(drogon::k400BadRequest);
        add_cors_headers(resp);
        callback(resp);
        return;
    }
    // Hash the password with bcrypt
    std::string password_hash;
    try {
        password_hash = orderbook::bcrypt_hash_password(password);
    } catch (const std::exception& ex) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value({{"error", "Failed to hash password securely"}}));
        resp->setStatusCode(drogon::k500InternalServerError);
        add_cors_headers(resp);
        callback(resp);
        return;
    }

    dbClient->execSqlAsync(
        "INSERT INTO users (username, password_hash) VALUES ($1, $2);",
        [callback](const drogon::orm::Result&) {
            auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value({{"result", "User registered"}}));
            add_cors_headers(resp);
            callback(resp);
        },
        [callback](const std::exception_ptr&) {
            auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value({{"error", "Username already exists"}}));
            resp->setStatusCode(drogon::k400BadRequest);
            add_cors_headers(resp);
            callback(resp);
        },
        username, password_hash
    );
}

void OrderBookController::loginUser(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto json = req->getJsonObject();
    if (!json || !(*json).isMember("username") || !(*json).isMember("password")) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value({{"error", "Missing username or password"}}));
        resp->setStatusCode(drogon::k400BadRequest);
        add_cors_headers(resp);
        callback(resp);
        return;
    }
    std::string username = (*json)["username"].asString();
    std::string password = (*json)["password"].asString();

    dbClient->execSqlAsync(
        "SELECT id, password_hash FROM users WHERE username=$1;",
        [callback, password, username](const drogon::orm::Result& result) {
            if (result.empty()) {
                auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value({{"error", "Invalid credentials"}}));
                resp->setStatusCode(drogon::k401Unauthorized);
                add_cors_headers(resp);
                callback(resp);
                return;
            }
            std::string hash = result[0]["password_hash"].as<std::string>();
            int user_id = result[0]["id"].as<int>();
            // Check the password with bcrypt
            bool valid = false;
            try {
                valid = orderbook::bcrypt_check_password(password, hash);
            } catch (const std::exception& ex) {
                valid = false;
            }
            if (!valid) {
                auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value({{"error", "Invalid credentials"}}));
                resp->setStatusCode(drogon::k401Unauthorized);
                add_cors_headers(resp);
                callback(resp);
                return;
            }
            // Issue JWT and return user info
            std::string token = jwt::create()
                .set_issuer("orderbook")
                .set_type("JWS")
                .set_payload_claim("user_id", jwt::claim(std::to_string(user_id)))
                .set_payload_claim("username", jwt::claim(username))
                .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(1))
                .sign(jwt::algorithm::hs256{orderbook::get_jwt_secret()});
            Json::Value json;
            json["token"] = token;
            Json::Value userJson;
            userJson["id"] = user_id;
            userJson["username"] = username;
            json["user"] = userJson;
            auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
            add_cors_headers(resp);
            callback(resp);
        },
        [callback](const std::exception_ptr&) {
            auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value({{"error", "Database error"}}));
            resp->setStatusCode(drogon::k500InternalServerError);
            add_cors_headers(resp);
            callback(resp);
        },
        username
    );
}

void OrderBookController::asyncDemo(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    // 1. Launch a background task with std::async
    auto future = std::async(std::launch::async, []() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return 42;
    });

    // 2. Launch a thread and use condition_variable for signaling
    std::thread t([]() {
        std::unique_lock<std::mutex> lock(demo_mutex);
        demo_shared_value = 99;
        demo_ready = true;
        demo_cv.notify_one();
    });
    t.detach();

    // 3. Wait for the signal
    {
        std::unique_lock<std::mutex> lock(demo_mutex);
        demo_cv.wait(lock, []{ return demo_ready; });
        demo_ready = false;
    }

    // 4. Use std::lock_guard for thread safety
    int safe_value;
    {
        std::lock_guard<std::mutex> lock(demo_mutex);
        safe_value = demo_shared_value;
    }

    // 5. Async DB access (if dbClient is set)
    if (dbClient) {
        dbClient->execSqlAsync(
            "SELECT 1",
            [callback, safe_value, future = std::move(future)](const drogon::orm::Result &result) mutable {
                Json::Value res;
                res["std_async_result"] = future.get();
                res["thread_safe_value"] = safe_value;
                res["db_result"] = result.empty() ? 0 : result[0][0].as<int>();
                auto resp = HttpResponse::newHttpJsonResponse(res);
                add_cors_headers(resp);
                callback(resp);
            },
            [callback](const std::exception_ptr &e) {
                Json::Value res;
                res["error"] = "DB error";
                auto resp = HttpResponse::newHttpJsonResponse(res);
                add_cors_headers(resp);
                callback(resp);
            }
        );
        return;
    }
    // If no DB, just return the async/thread results
    Json::Value res;
    res["std_async_result"] = future.get();
    res["thread_safe_value"] = safe_value;
    auto resp = HttpResponse::newHttpJsonResponse(res);
    add_cors_headers(resp);
    callback(resp);
}

// CORS preflight handlers - these respond to OPTIONS requests
void OrderBookController::handleOptions(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    auto resp = HttpResponse::newHttpResponse();
    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    resp->addHeader("Access-Control-Allow-Credentials", "true");
    resp->setStatusCode(k200OK);
    callback(resp);
}

void OrderBookController::handleOptionsWithParam(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string param) {
    auto resp = HttpResponse::newHttpResponse();
    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    resp->addHeader("Access-Control-Allow-Credentials", "true");
    resp->setStatusCode(k200OK);
    callback(resp);
}

void OrderBookController::clearAllOrders(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    try {
        // Clear all orders from the matching engine
        engine->clear();
        
        // Clear orders from database
        if (dbClient) {
            dbClient->execSqlAsync(
                "DELETE FROM orders;",
                [](const drogon::orm::Result& result) { /* Success */ },
                [](const std::exception_ptr& e) { /* Error */ }
            );
            dbClient->execSqlAsync(
                "DELETE FROM actions;",
                [](const drogon::orm::Result& result) { /* Success */ },
                [](const std::exception_ptr& e) { /* Error */ }
            );
        }
        
        // Broadcast empty order book to all WebSocket clients
        if (wsController) {
            wsController->broadcastOrderBook("BTCUSD");
        }
        
        Json::Value resj;
        resj["message"] = "All orders cleared successfully";
        resj["cleared"] = true;
        
        auto resp = HttpResponse::newHttpJsonResponse(resj);
        add_cors_headers(resp);
        callback(resp);
        
    } catch (const std::exception& e) {
        Json::Value errJson;
        errJson["error"] = "Failed to clear orders";
        auto resp = HttpResponse::newHttpJsonResponse(errJson);
        resp->setStatusCode(k500InternalServerError);
        add_cors_headers(resp);
        callback(resp);
    }
}
