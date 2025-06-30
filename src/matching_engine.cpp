#include "matching_engine.hpp"
#include <iostream> // For logging

namespace orderbook {

MatchingEngine::MatchingEngine() {}

std::vector<Trade> MatchingEngine::add_order(std::shared_ptr<Order> order) 
{
    std::unique_lock<std::shared_mutex> lock(engine_mutex_);
    auto [it, inserted] = order_books_.try_emplace(order->symbol, order->symbol);
    auto& book = it->second;
    // Set up callbacks for engine-wide events
    book.set_trade_callback([this](const Trade& t) {
        stats_.total_trades++;
        stats_.total_volume += t.quantity;
        add_trade_history(t); // Store trade in history
        if (on_trade) on_trade(t);
    });
    book.set_order_update_callback([this](const Order& o) {
        if (on_order_update) on_order_update(o);
    });
    auto trades = book.add_order(order);
    order_id_to_symbol_[order->id] = order->symbol;
    // Maintaining total_orders as a counter to avoid deadlock
    stats_.total_orders++;
    return trades;
}

bool MatchingEngine::cancel_order(OrderId order_id) 
{
    std::cout << "[LOG] MatchingEngine::cancel_order ENTER id=" << order_id << std::endl;
    std::unique_lock<std::shared_mutex> lock(engine_mutex_);
    auto it = order_id_to_symbol_.find(order_id);
    if (it == order_id_to_symbol_.end()) {
        std::cout << "[LOG] MatchingEngine::cancel_order NOT FOUND id=" << order_id << std::endl;
        return false;
    }
    auto [book_it, inserted] = order_books_.try_emplace(it->second, it->second);
    auto& book = book_it->second;
    bool result = book.cancel_order(order_id);
    if (result) 
    {
        order_id_to_symbol_.erase(it);
        if (stats_.total_orders > 0) stats_.total_orders--; // Decrement counter
    }
    std::cout << "[LOG] MatchingEngine::cancel_order EXIT id=" << order_id << std::endl;
    return result;
}

bool MatchingEngine::modify_order(OrderId order_id, Price new_price, Quantity new_quantity) {
    std::shared_lock<std::shared_mutex> lock(engine_mutex_);
    auto it = order_id_to_symbol_.find(order_id);
    if (it == order_id_to_symbol_.end()) return false;
    auto [book_it, inserted] = order_books_.try_emplace(it->second, it->second);
    auto& book = book_it->second;
    return book.modify_order(order_id, new_price, new_quantity);
}

std::shared_ptr<Order> MatchingEngine::get_order(OrderId order_id) const {
    std::shared_lock<std::shared_mutex> lock(engine_mutex_);
    auto it = order_id_to_symbol_.find(order_id);
    if (it == order_id_to_symbol_.end()) return nullptr;
    const auto& book = order_books_.at(it->second);
    return book.get_order(order_id);
}

Price MatchingEngine::get_best_bid(const std::string& symbol) const {
    std::shared_lock<std::shared_mutex> lock(engine_mutex_);
    auto it = order_books_.find(symbol);
    return (it != order_books_.end()) ? it->second.get_best_bid() : 0;
}

Price MatchingEngine::get_best_ask(const std::string& symbol) const {
    std::shared_lock<std::shared_mutex> lock(engine_mutex_);
    auto it = order_books_.find(symbol);
    return (it != order_books_.end()) ? it->second.get_best_ask() : 0;
}

Price MatchingEngine::get_spread(const std::string& symbol) const {
    std::shared_lock<std::shared_mutex> lock(engine_mutex_);
    auto it = order_books_.find(symbol);
    return (it != order_books_.end()) ? it->second.get_spread() : 0;
}

std::vector<OrderBookLevel> MatchingEngine::get_bid_levels(const std::string& symbol, size_t depth) const {
    std::shared_lock<std::shared_mutex> lock(engine_mutex_);
    auto it = order_books_.find(symbol);
    return (it != order_books_.end()) ? it->second.get_bid_levels(depth) : std::vector<OrderBookLevel>{};
}

std::vector<OrderBookLevel> MatchingEngine::get_ask_levels(const std::string& symbol, size_t depth) const {
    std::shared_lock<std::shared_mutex> lock(engine_mutex_);
    auto it = order_books_.find(symbol);
    return (it != order_books_.end()) ? it->second.get_ask_levels(depth) : std::vector<OrderBookLevel>{};
}

size_t MatchingEngine::get_order_count() const {
    // Use the counter instead of recalculating
    return stats_.total_orders;
}

std::vector<std::shared_ptr<Order>> MatchingEngine::get_all_orders() const {
    std::shared_lock<std::shared_mutex> lock(engine_mutex_);
    std::vector<std::shared_ptr<Order>> all_orders;
    for (const auto& [_, book] : order_books_) {
        for (const auto& level : book.get_bid_levels(1000)) {
            for (const auto& order : level.orders) {
                all_orders.push_back(order);
            }
        }
        for (const auto& level : book.get_ask_levels(1000)) {
            for (const auto& order : level.orders) {
                all_orders.push_back(order);
            }
        }
    }
    return all_orders;
}

std::vector<std::shared_ptr<Order>> MatchingEngine::get_user_orders(const std::string& user_id) const {
    std::shared_lock<std::shared_mutex> lock(engine_mutex_);
    std::vector<std::shared_ptr<Order>> user_orders;

    for (const auto& [_, book] : order_books_) {
        for (const auto& level : book.get_bid_levels(1000)) {
            for (const auto& order : level.orders) {
                if (order->user_id == user_id) {
                    user_orders.push_back(order);
                }
            }
        }
        for (const auto& level : book.get_ask_levels(1000)) {
            for (const auto& order : level.orders) {
                if (order->user_id == user_id) {
                    user_orders.push_back(order);
                }
            }
        }
    }

    return user_orders;
}

std::vector<Trade> MatchingEngine::get_user_trades(const std::string& user_id) const {
    std::shared_lock<std::shared_mutex> lock(engine_mutex_);
    std::vector<Trade> trades;
    for (const auto& [_, book] : order_books_) {
        auto user_trades = book.get_user_trades(user_id);
        trades.insert(trades.end(), user_trades.begin(), user_trades.end());
    }
    return trades;
}

EngineStats MatchingEngine::get_stats() const {
    std::shared_lock<std::shared_mutex> lock(engine_mutex_);
    return stats_;
}

void MatchingEngine::clear() {
    std::unique_lock<std::shared_mutex> lock(engine_mutex_);
    for (auto& [_, book] : order_books_) {
        book.clear();
    }
    order_books_.clear();
    order_id_to_symbol_.clear();
}

void MatchingEngine::cancel_expired_orders() {
    std::unique_lock<std::shared_mutex> lock(engine_mutex_);
    for (auto& [_, book] : order_books_) {
        book.cancel_expired_orders();
    }
}

void MatchingEngine::add_trade_history(const Trade& trade) {
    trade_history_.push_back(trade);
}

} // namespace orderbook
