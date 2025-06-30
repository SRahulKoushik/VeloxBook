#include <gtest/gtest.h>
#include <drogon/drogon.h>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpClient.h>
#include "OrderBookController.h"
#include "matching_engine.hpp"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <iostream>

using namespace drogon;
using namespace orderbook;

// Fixture to start/stop the Drogon server for fuzz tests
class OrderBookFuzzServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Start Drogon app in a background thread on port 18082
        appThread = std::thread([]() {
            static MatchingEngine engine;
            // Set the shared engine pointer or reference in controller
            OrderBookController::setEngine(&engine);
            // Listen on 127.0.0.1:18082 (adjust port if needed)
            drogon::app().addListener("127.0.0.1", 18082);
            drogon::app().run();
        });
        // Wait briefly for server to start up (adjust duration if startup is slower)
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    void TearDown() override {
        // Signal server to quit and join thread
        drogon::app().quit();
        if (appThread.joinable()) {
            appThread.join();
        }
    }

    std::thread appThread;
};

// Fuzz test: Place, cancel, and modify many orders concurrently
TEST_F(OrderBookFuzzServerTest, FuzzCancelAndModify) {
    // Create a single HttpClient instance; Drogon HttpClient is thread-safe for concurrent requests
    auto client = HttpClient::newHttpClient("http://127.0.0.1:18082");

    const int N = 50;
    std::vector<std::string> order_ids;
    order_ids.reserve(N);

    // 1) Place N orders sequentially to collect order IDs
    for (int i = 0; i < N; ++i) {
        Json::Value body;
        body["symbol"] = "BTCUSD";
        body["side"] = (i % 2 == 0) ? "buy" : "sell";
        body["type"] = "limit";
        body["price"] = 10000 + (i % 10);
        body["quantity"] = 1;
        body["user_id"] = "user" + std::to_string(i);

        auto req = HttpRequest::newHttpJsonRequest(body);
        req->setMethod(Post);
        req->setPath("/order");  // adjust path if different

        auto resp = client->sendRequest(req);
        // resp.first is ReqResult, resp.second is HttpResponsePtr
        ASSERT_EQ(resp.first, ReqResult::Ok);
        ASSERT_NE(resp.second, nullptr);
        ASSERT_EQ(resp.second->statusCode(), k200OK);

        // Parse JSON response to extract "order_id"
        Json::Value j;
        Json::CharReaderBuilder rbuilder;
        std::string errs;
        std::istringstream ss(std::string(resp.second->body()));
        ASSERT_TRUE(Json::parseFromStream(rbuilder, ss, &j, &errs)) << "JSON parse error: " << errs;
        ASSERT_TRUE(j.isMember("order_id")) << "Response missing order_id";
        order_ids.push_back(j["order_id"].asString());
    }

    // 2) Concurrently cancel or modify each order
    std::vector<std::thread> threads;
    threads.reserve(N);
    std::atomic<int> cancels{0}, modifies{0};

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&, i]() {
            if (i % 2 == 0) {
                // Cancel endpoint: DELETE /cancel/{order_id}
                auto cancelPath = "/cancel/" + order_ids[i];
                auto req = HttpRequest::newHttpRequest();
                req->setMethod(Delete);
                req->setPath(cancelPath);
                auto resp = client->sendRequest(req);
                if (resp.first == ReqResult::Ok && resp.second && resp.second->statusCode() == k200OK) {
                    cancels.fetch_add(1, std::memory_order_relaxed);
                }
            } else {
                // Modify endpoint: POST /modify with JSON body
                Json::Value body;
                body["order_id"] = order_ids[i];
                body["price"] = 10000 + (i % 10) + 1;
                body["quantity"] = 2;
                auto req = HttpRequest::newHttpJsonRequest(body);
                req->setMethod(Post);
                req->setPath("/modify");
                auto resp = client->sendRequest(req);
                if (resp.first == ReqResult::Ok && resp.second && resp.second->statusCode() == k200OK) {
                    modifies.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    // Join all threads
    for (auto &t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    // Expect total successful operations = N
    int total = cancels.load(std::memory_order_relaxed) + modifies.load(std::memory_order_relaxed);
    ASSERT_EQ(total, N) << "Expected " << N << " successful operations, got " << total;
}




