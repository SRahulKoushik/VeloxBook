#include <gtest/gtest.h>
#include "OrderBookController.h"
#include "matching_engine.hpp"
#include "order.hpp"
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <memory>
#include <string>

using namespace orderbook;
using namespace drogon;

class OrderBookControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine = std::make_unique<MatchingEngine>();
        OrderBookController::setEngine(engine.get());
    }
    std::unique_ptr<MatchingEngine> engine;
};

TEST_F(OrderBookControllerTest, PlaceOrder_Valid) {
    OrderBookController controller;
    auto req = HttpRequest::newHttpRequest();
    Json::Value body;
    body["symbol"] = "BTCUSD";
    body["side"] = "buy";
    body["type"] = "limit";
    body["price"] = 10000;
    body["quantity"] = 1;
    body["user_id"] = "alice";
    Json::StreamWriterBuilder wbuilder;
    req->setBody(Json::writeString(wbuilder, body));
    bool called = false;
    controller.placeOrder(req, [&](const HttpResponsePtr& resp) {
        ASSERT_EQ(resp->statusCode(), k200OK);
        Json::Value j;
        Json::CharReaderBuilder rbuilder;
        std::string errs;
        std::istringstream s(std::string(resp->body()));
        ASSERT_TRUE(Json::parseFromStream(rbuilder, s, &j, &errs));
        ASSERT_EQ(j["status"].asString(), "open");
        ASSERT_TRUE(j["order_id"].isString());
        called = true;
    });
    ASSERT_TRUE(called);
}

TEST_F(OrderBookControllerTest, PlaceOrder_Invalid) {
    OrderBookController controller;
    auto req = HttpRequest::newHttpRequest();
    Json::Value body;
    body["symbol"] = "BTCUSD";
    body["side"] = "buy";
    body["type"] = "limit";
    body["quantity"] = 1;
    body["user_id"] = "alice"; // missing price
    Json::StreamWriterBuilder wbuilder;
    req->setBody(Json::writeString(wbuilder, body));
    bool called = false;
    controller.placeOrder(req, [&](const HttpResponsePtr& resp) {
        ASSERT_EQ(resp->statusCode(), k400BadRequest);
        Json::Value j;
        Json::CharReaderBuilder rbuilder;
        std::string errs;
        std::istringstream s(std::string(resp->body()));
        ASSERT_TRUE(Json::parseFromStream(rbuilder, s, &j, &errs));
        ASSERT_TRUE(j["error"].isString());
        called = true;
    });
    ASSERT_TRUE(called);
}

TEST_F(OrderBookControllerTest, CancelOrder_NotFound) {
    OrderBookController controller;
    auto req = HttpRequest::newHttpRequest();
    bool called = false;
    controller.cancelOrder(req, [&](const HttpResponsePtr& resp) {
        ASSERT_EQ(resp->statusCode(), k404NotFound);
        Json::Value j;
        Json::CharReaderBuilder rbuilder;
        std::string errs;
        std::istringstream s(std::string(resp->body()));
        ASSERT_TRUE(Json::parseFromStream(rbuilder, s, &j, &errs));
        ASSERT_TRUE(j["error"].isString());
        called = true;
    }, "nonexistent");
    ASSERT_TRUE(called);
}

TEST_F(OrderBookControllerTest, GetOrders_Empty) {
    OrderBookController controller;
    auto req = HttpRequest::newHttpRequest();
    bool called = false;
    controller.getOrders(req, [&](const HttpResponsePtr& resp) {
        ASSERT_EQ(resp->statusCode(), k200OK);
        Json::Value j;
        Json::CharReaderBuilder rbuilder;
        std::string errs;
        std::istringstream s(std::string(resp->body()));
        ASSERT_TRUE(Json::parseFromStream(rbuilder, s, &j, &errs));
        ASSERT_TRUE(j["orders"].isArray());
        ASSERT_EQ(j["orders"].size(), 0);
        called = true;
    }, "alice");
    ASSERT_TRUE(called);
}

// Add more tests for modifyOrder, getOrderById, getTradeHistory, etc.
