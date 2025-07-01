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
class OrderBookController : public drogon::HttpController<OrderBookController> {
public:
    static constexpr bool isAutoCreation = false;
    
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(OrderBookController::placeOrder, "/order", Post, Options);
    ADD_METHOD_TO(OrderBookController::cancelOrder, "/cancel/{1}", Delete, Options);
    ADD_METHOD_TO(OrderBookController::modifyOrder, "/modify", Post, Options);
    ADD_METHOD_TO(OrderBookController::getOrders, "/orders/{1}", Get, Options);
    ADD_METHOD_TO(OrderBookController::getOrderBook, "/orderbook/{1}", Get, Options);
    ADD_METHOD_TO(OrderBookController::health, "/health", Get, Options);
    ADD_METHOD_TO(OrderBookController::metrics, "/metrics", Get, Options);
    // New endpoints
    ADD_METHOD_TO(OrderBookController::getOrderById, "/order/{1}", Get, Options);
    ADD_METHOD_TO(OrderBookController::getTradeHistory, "/trades/{1}", Get, Options);
    ADD_METHOD_TO(OrderBookController::registerUser, "/register", Post, Options);
    ADD_METHOD_TO(OrderBookController::loginUser, "/login", Post, Options);
    ADD_METHOD_TO(OrderBookController::asyncDemo, "/async_demo", Get, Options);
    METHOD_LIST_END

    void placeOrder(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    void cancelOrder(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string orderId);
    void modifyOrder(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    void getOrders(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string userId);
    void getOrderBook(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string symbol);
    void health(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    void metrics(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    // New endpoints
    void getOrderById(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string orderId);
    void getTradeHistory(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string userId);
    void registerUser(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void loginUser(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void asyncDemo(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);

    static void setEngine(MatchingEngine* eng);
    static void setWebSocketController(OrderBookWebSocket* ws);
    static void setMetrics(std::atomic<size_t>* oc, std::atomic<size_t>* tc, std::atomic<double>* lat);
    static void setDbClient(drogon::orm::DbClientPtr dbClient_);
private:
    static MatchingEngine* engine;
    static OrderBookWebSocket* wsController;
    static std::atomic<size_t>* g_order_count;
    static std::atomic<size_t>* g_trade_count;
    static std::atomic<double>* g_last_order_latency_ms;
    static drogon::orm::DbClientPtr dbClient;
};
