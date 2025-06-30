#ifndef ORDERBOOK_ORDER_BOOK_HPP
#define ORDERBOOK_ORDER_BOOK_HPP

#include "order.hpp"
#include <map>
#include <deque>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <vector>

namespace orderbook 
{

// Level in the order book (single price point)
struct OrderBookLevel {
    Price price;
    std::deque<std::shared_ptr<Order>> orders;
    Quantity total_quantity = 0;

    OrderBookLevel() = default;
    explicit OrderBookLevel(Price price_) : price(price_) {}
};

// Order book class
class OrderBook {
public:
    explicit OrderBook(const std::string& symbol);

    std::vector<Trade> add_order(std::shared_ptr<Order> order);
    bool cancel_order(OrderId order_id);
    bool modify_order(OrderId order_id, Price new_price, Quantity new_quantity);

    Price get_best_bid() const;
    Price get_best_ask() const;
    Price get_spread() const;

    Quantity get_bid_depth(Price price) const;
    Quantity get_ask_depth(Price price) const;

    std::vector<OrderBookLevel> get_bid_levels(size_t depth) const;
    std::vector<OrderBookLevel> get_ask_levels(size_t depth) const;

    std::shared_ptr<Order> get_order(OrderId order_id) const;
    void clear();
    bool is_empty() const;
    size_t get_order_count() const;

    void set_order_update_callback(std::function<void(const Order&)> cb) {
        order_update_callback_ = std::move(cb);
    }

    void set_trade_callback(std::function<void(const Trade&)> cb) {
        trade_callback_ = std::move(cb);
    }

    // --- Metrics ---
    double average_spread(size_t depth = 10) const;
    double order_to_trade_ratio() const;
    double cancellation_rate() const;

    // --- Expiry and TIF ---
    void cancel_expired_orders();

    // --- Trade history ---
    const std::vector<Trade>& get_trade_history() const { return trade_history_; }
    std::vector<Trade> get_user_trades(const std::string& user_id) const {
        std::vector<Trade> result;
        for (const auto& t : trade_history_) {
            // Check if user is buyer or seller
            if (orders_by_id_.count(t.buy_order_id) && orders_by_id_.at(t.buy_order_id)->user_id == user_id)
                result.push_back(t);
            else if (orders_by_id_.count(t.sell_order_id) && orders_by_id_.at(t.sell_order_id)->user_id == user_id)
                result.push_back(t);
        }
        return result;
    }
private:
    std::string symbol_;

    // Buy orders sorted descending (highest price first)
    std::map<Price, OrderBookLevel, std::greater<>> buy_orders_;
    // Sell orders sorted ascending (lowest price first)
    std::map<Price, OrderBookLevel> sell_orders_;
    std::map<OrderId, std::shared_ptr<Order>> orders_by_id_;
    mutable std::shared_mutex order_book_mutex_;
    mutable std::shared_mutex orders_mutex_;
    std::atomic<size_t> total_orders_{0};
    std::atomic<size_t> total_trades_{0};
    std::atomic<Quantity> total_volume_{0};
    std::function<void(const Order&)> order_update_callback_;
    std::function<void(const Trade&)> trade_callback_;
    std::vector<Trade> match_orders(std::shared_ptr<Order> order);
    void add_order_to_level(std::shared_ptr<Order> order);
    void remove_order_from_level(std::shared_ptr<Order> order);
    void process_market_order(std::shared_ptr<Order> order);
    void process_limit_order(std::shared_ptr<Order> order);
    void process_stop_order(std::shared_ptr<Order> order);
    void process_stop_limit_order(std::shared_ptr<Order> order);
    std::vector<Trade> trade_history_;
};

} // namespace orderbook

#endif // ORDERBOOK_ORDER_BOOK_HPP
