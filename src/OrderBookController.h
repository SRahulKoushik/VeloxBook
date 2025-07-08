#pragma once
#include <drogon/HttpController.h>
#include <drogon/orm/DbClient.h>
#include "matching_engine.hpp"
#include <memory>
#include <string>
#include <atomic>

using namespace drogon;
using namespace orderbook;

class OrderBookWebSocket;

// Main API controller for the trading platform
// Handles all the HTTP endpoints that the frontend calls
class OrderBookController : public drogon::HttpController<OrderBookController> {
public:
    static constexpr bool isAutoCreation = false;
    
    // Define all the API endpoints
    // The frontend calls these URLs to interact with the trading system
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(OrderBookController::placeOrder, "/api/order", Post, Options);
    ADD_METHOD_TO(OrderBookController::cancelOrder, "/api/cancel/{1}", Delete, Options);
    ADD_METHOD_TO(OrderBookController::modifyOrder, "/api/modify", Post, Options);
    ADD_METHOD_TO(OrderBookController::getOrders, "/api/orders/{1}", Get, Options);
    ADD_METHOD_TO(OrderBookController::getOrderBook, "/api/orderbook/{1}", Get, Options);
    ADD_METHOD_TO(OrderBookController::health, "/api/health", Get, Options);
    ADD_METHOD_TO(OrderBookController::metrics, "/api/metrics", Get, Options);
    // User management and additional features
    ADD_METHOD_TO(OrderBookController::getOrderById, "/api/order/{1}", Get, Options);
    ADD_METHOD_TO(OrderBookController::getTradeHistory, "/api/trades/{1}", Get, Options);
    ADD_METHOD_TO(OrderBookController::registerUser, "/api/register", Post, Options);
    ADD_METHOD_TO(OrderBookController::loginUser, "/api/login", Post, Options);
    ADD_METHOD_TO(OrderBookController::asyncDemo, "/api/async_demo", Get, Options);
    ADD_METHOD_TO(OrderBookController::clearAllOrders, "/api/clear-orders", Delete, Options);
    
    // Explicit OPTIONS handlers for CORS
    ADD_METHOD_TO(OrderBookController::handleOptions, "/api/order", Options);
    ADD_METHOD_TO(OrderBookController::handleOptions, "/api/modify", Options);
    ADD_METHOD_TO(OrderBookController::handleOptions, "/api/health", Options);
    ADD_METHOD_TO(OrderBookController::handleOptions, "/api/metrics", Options);
    ADD_METHOD_TO(OrderBookController::handleOptions, "/api/register", Options);
    ADD_METHOD_TO(OrderBookController::handleOptions, "/api/login", Options);
    ADD_METHOD_TO(OrderBookController::handleOptions, "/api/async_demo", Options);
    ADD_METHOD_TO(OrderBookController::handleOptions, "/api/clear-orders", Options);
    ADD_METHOD_TO(OrderBookController::handleOptionsWithParam, "/api/cancel/{1}", Options);
    ADD_METHOD_TO(OrderBookController::handleOptionsWithParam, "/api/orders/{1}", Options);
    ADD_METHOD_TO(OrderBookController::handleOptionsWithParam, "/api/orderbook/{1}", Options);
    ADD_METHOD_TO(OrderBookController::handleOptionsWithParam, "/api/order/{1}", Options);
    ADD_METHOD_TO(OrderBookController::handleOptionsWithParam, "/api/trades/{1}", Options);
    METHOD_LIST_END

    // Core trading endpoints
    void placeOrder(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    void cancelOrder(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string orderId);
    void modifyOrder(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    void getOrders(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string userId);
    void getOrderBook(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string symbol);
    void health(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    void metrics(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    // User management and additional features
    void getOrderById(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string orderId);
    void getTradeHistory(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string userId);
    void registerUser(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void loginUser(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void asyncDemo(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    void clearAllOrders(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);

    // CORS preflight handlers
    void handleOptions(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    void handleOptionsWithParam(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string param);

    // Setup methods - called during initialization
    static void setEngine(MatchingEngine* eng);
    static void setWebSocketController(OrderBookWebSocket* ws);
    static void setMetrics(std::atomic<size_t>* oc, std::atomic<size_t>* tc, std::atomic<double>* lat);
    static void setDbClient(drogon::orm::DbClientPtr dbClient_);
private:
    // Shared components that all controller instances can access
    static MatchingEngine* engine;
    static OrderBookWebSocket* wsController;
    static std::atomic<size_t>* g_order_count;
    static std::atomic<size_t>* g_trade_count;
    static std::atomic<double>* g_last_order_latency_ms;
    static drogon::orm::DbClientPtr dbClient;
};
