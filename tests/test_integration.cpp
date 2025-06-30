// tests/test_integration.cpp

#include <gtest/gtest.h>
#include <drogon/drogon.h>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpClient.h>
#include <drogon/WebSocketClient.h>
#include <json/json.h>
#include "OrderBookController.h"
#include "OrderBookWebSocket.h"
#include "matching_engine.hpp"
#include "order.hpp"
#include <thread>
#include <future>
#include <sstream>
#include <atomic>
#include <chrono>
#include <iostream>

using namespace drogon;
using namespace orderbook;

// Fixture: start and stop the Drogon server for each test
class OrderBookIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Start Drogon app in a background thread on port 18081
        appThread = std::thread([]() {
            static MatchingEngine engine;
            OrderBookController::setEngine(&engine);
            drogon::app().addListener("127.0.0.1", 18081);
            drogon::app().run();
        });
        // Wait briefly for server startup
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    void TearDown() override {
        drogon::app().quit();
        if (appThread.joinable()) {
            appThread.join();
        }
    }

    std::thread appThread;
};

// Test 1: PlaceOrderEndpoint
TEST_F(OrderBookIntegrationTest, PlaceOrderEndpoint) {
    auto client = HttpClient::newHttpClient("http://127.0.0.1:18081");

    // Build JSON body
    Json::Value body;
    body["symbol"] = "BTCUSD";
    body["side"] = "buy";
    body["type"] = "limit";
    body["price"] = 10000;
    body["quantity"] = 1;
    body["user_id"] = "alice";

    // Create JSON request
    auto req = HttpRequest::newHttpJsonRequest(body);
    req->setMethod(Post);
    req->setPath("/order");

    // Send request
    auto resp = client->sendRequest(req);
    ASSERT_EQ(resp.first, ReqResult::Ok);
    ASSERT_NE(resp.second, nullptr);
    ASSERT_EQ(resp.second->statusCode(), k200OK);

    // Parse JSON response
    Json::Value j;
    Json::CharReaderBuilder rbuilder;
    std::string errs;
    std::istringstream ss(std::string(resp.second->body()));
    ASSERT_TRUE(Json::parseFromStream(rbuilder, ss, &j, &errs)) << "JSON parse error: " << errs;
    ASSERT_TRUE(j.isMember("status"));
    ASSERT_EQ(j["status"].asString(), "open");
}

// Test 2: WebSocketBroadcast
TEST_F(OrderBookIntegrationTest, WebSocketBroadcast) {
    std::promise<WebSocketConnectionPtr> connPromise;
    auto connFuture = connPromise.get_future();
    std::atomic<bool> connSet{false};
    std::promise<std::string> msgPromise;
    auto msgFuture = msgPromise.get_future();

    auto wsClient = WebSocketClient::newWebSocketClient("ws://127.0.0.1:18081");
    auto wsReq = HttpRequest::newHttpRequest();
    wsReq->setPath("/ws/orderbook");

    wsClient->connectToServer(
        wsReq,
        [&connPromise, &connSet](ReqResult result,
                                const HttpResponsePtr &resp,
                                const WebSocketClientPtr &client) {
            if (result == ReqResult::Ok && client && !connSet.exchange(true)) {
                connPromise.set_value(client->getConnection());
            }
        }
    );


    auto status = connFuture.wait_for(std::chrono::seconds(2));
    ASSERT_EQ(status, std::future_status::ready) << "WebSocket connection timed out";
    WebSocketConnectionPtr wsConn = connFuture.get();
    ASSERT_TRUE(wsConn) << "WebSocket connection failed";

    // Trigger broadcast via HTTP: place an order
    auto client = HttpClient::newHttpClient("http://127.极客0.1:18081");
    Json::Value body;
    body["symbol"] = "BTCUSD";
    body["side"] = "buy";
    body["type"] = "limit";
    body["price"] = 10001;
    body["quantity"] = 1;
    body["user_id"] = "bob";

    auto req = HttpRequest::newHttpJsonRequest(body);
    req->setMethod(Post);
    req->setPath("/order");
    auto resp = client->sendRequest(req);
    ASSERT_EQ(resp.first, ReqResult::Ok);
    ASSERT_NE(resp.second, nullptr);
    ASSERT_EQ(resp.second->statusCode(), k200OK);

    auto wstatus = msgFuture.wait_for(std::chrono::seconds(2));
    ASSERT_EQ(wstatus, std::future_status::ready) << "Did not receive WebSocket broadcast in time";
    std::string wsMsg = msgFuture.get();

    Json::Value j;
    Json::CharReaderBuilder rbuilder;
    std::string errs;
    std::istringstream ss(wsMsg);
    ASSERT_TRUE(Json::parseFromStream(rbuilder, ss, &j, &errs)) << "WS JSON parse error: " << errs;
    ASSERT_TRUE(j.isMember("type"));
    ASSERT_EQ(j["type"].asString(), "orderbook");
}

// Test 3: FuzzPlaceOrders (concurrent load test)
TEST_F(OrderBookIntegrationTest, FuzzPlaceOrders) {
    auto client = HttpClient::newHttpClient("http://127.0.0.1:18081");

    const int N = 100;
    std::vector<std::thread> threads;
    threads.reserve(N);
    std::atomic<int> successCount{0};

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&, i]() {
            Json::Value body;
            body["symbol"] = "BTCUSD";
            body["side"] = (i % 2 == 0) ? "buy" : "sell";
            body["type"] = "limit";
            body["price"] = 10000 + (i % 10);
            body["quantity"] = 1;
            body["user_id"] = "user" + std::to_string(i);

            auto req = HttpRequest::newHttpJsonRequest(body);
            req->setMethod(Post);
            req->setPath("/order");

            auto resp = client->sendRequest(req);
            if (resp.first == ReqResult::Ok && resp.second && resp.second->statusCode() == k200OK) {
                successCount.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto &t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    ASSERT_EQ(successCount.load(std::memory_order_relaxed), N);
}


