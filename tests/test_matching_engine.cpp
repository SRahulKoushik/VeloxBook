#include <gtest/gtest.h>
#include "matching_engine.hpp"
#include "order.hpp"
#include <string>
#include <memory>
#include <vector>

using namespace orderbook;

class MatchingEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine = std::make_shared<MatchingEngine>();
    }
    std::shared_ptr<MatchingEngine> engine;
};

TEST_F(MatchingEngineTest, AddOrderAndGetOrder) {
    auto order = std::make_shared<Order>("1", "BTCUSD", OrderSide::BUY, OrderType::LIMIT, 10000, 1, "alice");
    auto trades = engine->add_order(order);
    ASSERT_TRUE(trades.empty());
    auto fetched = engine->get_order("1");
    ASSERT_NE(fetched, nullptr);
    ASSERT_EQ(fetched->id, "1");
    ASSERT_EQ(fetched->symbol, "BTCUSD");
    ASSERT_EQ(fetched->side, OrderSide::BUY);
}

TEST_F(MatchingEngineTest, CancelOrder) {
    auto order = std::make_shared<Order>("2", "BTCUSD", OrderSide::SELL, OrderType::LIMIT, 10010, 1, "bob");
    engine->add_order(order);
    ASSERT_TRUE(engine->cancel_order("2"));
    ASSERT_FALSE(engine->cancel_order("2"));
}

TEST_F(MatchingEngineTest, ModifyOrder) {
    auto order = std::make_shared<Order>("3", "BTCUSD", OrderSide::BUY, OrderType::LIMIT, 10000, 1, "alice");
    engine->add_order(order);
    ASSERT_TRUE(engine->modify_order("3", 10100, 2));
    auto fetched = engine->get_order("3");
    ASSERT_EQ(fetched->price, 10100);
    ASSERT_EQ(fetched->quantity, 2);
}

TEST_F(MatchingEngineTest, GetUserOrders) {
    engine->add_order(std::make_shared<Order>("4", "BTCUSD", OrderSide::BUY, OrderType::LIMIT, 10000, 1, "alice"));
    engine->add_order(std::make_shared<Order>("5", "BTCUSD", OrderSide::SELL, OrderType::LIMIT, 10010, 1, "bob"));
    auto alice_orders = engine->get_user_orders("alice");
    ASSERT_EQ(alice_orders.size(), 1);
    ASSERT_EQ(alice_orders[0]->user_id, "alice");
}

TEST_F(MatchingEngineTest, GetAllOrders) {
    engine->add_order(std::make_shared<Order>("6", "BTCUSD", OrderSide::BUY, OrderType::LIMIT, 10000, 1, "alice"));
    engine->add_order(std::make_shared<Order>("7", "BTCUSD", OrderSide::SELL, OrderType::LIMIT, 10010, 1, "bob"));
    auto all_orders = engine->get_all_orders();
    ASSERT_GE(all_orders.size(), 2);
}

TEST_F(MatchingEngineTest, TradeMatching) {
    engine->add_order(std::make_shared<Order>("8", "BTCUSD", OrderSide::SELL, OrderType::LIMIT, 10000, 1, "bob"));
    auto buy_order = std::make_shared<Order>("9", "BTCUSD", OrderSide::BUY, OrderType::LIMIT, 10000, 1, "alice");
    auto trades = engine->add_order(buy_order);
    ASSERT_EQ(trades.size(), 1);
    ASSERT_EQ(trades[0].price, 10000);
    ASSERT_EQ(trades[0].quantity, 1);
}

TEST_F(MatchingEngineTest, ExpiryAndCancelExpiredOrders) {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto expired_order = std::make_shared<Order>("10", "BTCUSD", OrderSide::BUY, OrderType::LIMIT, 10000, 1, "alice", 0, now - 10, "GTC");
    engine->add_order(expired_order);
    engine->cancel_expired_orders();
    auto fetched = engine->get_order("10");
    ASSERT_EQ(fetched, nullptr);
}

TEST_F(MatchingEngineTest, GetUserTrades) {
    engine->add_order(std::make_shared<Order>("11", "BTCUSD", OrderSide::SELL, OrderType::LIMIT, 10000, 1, "bob"));
    auto buy_order = std::make_shared<Order>("12", "BTCUSD", OrderSide::BUY, OrderType::LIMIT, 10000, 1, "alice");
    engine->add_order(buy_order);
    auto trades = engine->get_user_trades("alice");
    ASSERT_EQ(trades.size(), 1);
    ASSERT_EQ(trades[0].buy_order_id, "12");
}

TEST_F(MatchingEngineTest, BestBidAskAndSpread) {
    engine->add_order(std::make_shared<Order>("13", "BTCUSD", OrderSide::BUY, OrderType::LIMIT, 9990, 1, "alice"));
    engine->add_order(std::make_shared<Order>("14", "BTCUSD", OrderSide::SELL, OrderType::LIMIT, 10010, 1, "bob"));
    ASSERT_EQ(engine->get_best_bid("BTCUSD"), 9990);
    ASSERT_EQ(engine->get_best_ask("BTCUSD"), 10010);
    ASSERT_EQ(engine->get_spread("BTCUSD"), 20);
}

TEST_F(MatchingEngineTest, MetricsAndStats) {
    auto stats = engine->get_stats();
    ASSERT_EQ(stats.total_orders, 0);
    engine->add_order(std::make_shared<Order>("15", "BTCUSD", OrderSide::BUY, OrderType::LIMIT, 10000, 1, "alice"));
    stats = engine->get_stats();
    ASSERT_EQ(stats.total_orders, 1);
}

TEST_F(MatchingEngineTest, OrderBookDepth) {
    for (int i = 0; i < 5; ++i) {
        engine->add_order(std::make_shared<Order>("b" + std::to_string(i), "BTCUSD", OrderSide::BUY, OrderType::LIMIT, 10000 + i, 1, "alice"));
        engine->add_order(std::make_shared<Order>("s" + std::to_string(i), "BTCUSD", OrderSide::SELL, OrderType::LIMIT, 10010 + i, 1, "bob"));
    }
    auto bids = engine->get_bid_levels("BTCUSD", 5);
    auto asks = engine->get_ask_levels("BTCUSD", 5);
    ASSERT_EQ(bids.size(), 5);
    ASSERT_EQ(asks.size(), 5);
}

TEST_F(MatchingEngineTest, ClearEngine) {
    engine->add_order(std::make_shared<Order>("16", "BTCUSD", OrderSide::BUY, OrderType::LIMIT, 10000, 1, "alice"));
    engine->clear();
    ASSERT_EQ(engine->get_all_orders().size(), 0);
}

// Add more tests for other functionalities as needed.
