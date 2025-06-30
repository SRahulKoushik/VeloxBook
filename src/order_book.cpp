#include "order_book.hpp"
#include <algorithm>
#include <iostream>
#include <chrono>
#include <cassert>

namespace orderbook 
{

OrderBook::OrderBook(const std::string& symbol) : symbol_(symbol) {}

std::vector<Trade> OrderBook::add_order(std::shared_ptr<Order> order) {
    std::vector<Trade> trades;

    // Validate order
    if (order->quantity == 0 || order->quantity > MAX_ORDER_QUANTITY ||
        (order->type == OrderType::LIMIT && (order->price == 0 || order->price > MAX_ORDER_PRICE))) {
        order->status = OrderStatus::REJECTED;
        return trades;
    }

    {
        std::unique_lock lock(orders_mutex_);
        orders_by_id_[order->id] = order;
    }

    total_orders_++;

    switch (order->type) {
        case OrderType::MARKET:
            process_market_order(order);
            break;
        case OrderType::LIMIT:
            trades = match_orders(order);
            if (order->quantity > order->filled_quantity) {
                process_limit_order(order);
            }
            break;
        case OrderType::STOP:
            process_stop_order(order);
            break;
        case OrderType::STOP_LIMIT:
            process_stop_limit_order(order);
            break;
    }

    if (order->filled_quantity == order->quantity) {
        order->status = OrderStatus::FILLED;
    } else if (order->filled_quantity > 0) {
        order->status = OrderStatus::PARTIAL;
    }

    if (order_update_callback_) {
        order_update_callback_(*order);
    }

    // Store trades in trade_history_
    trade_history_.insert(trade_history_.end(), trades.begin(), trades.end());

    return trades;
}

std::vector<Trade> OrderBook::match_orders(std::shared_ptr<Order> order) {
    std::vector<Trade> trades;
    std::unique_lock lock(order_book_mutex_);

    bool is_buy = (order->side == OrderSide::BUY);
    if (is_buy) {
        // Match against sell orders
        while (order->filled_quantity < order->quantity && !sell_orders_.empty()) {
            auto it = sell_orders_.begin();
            auto price = it->first;
            auto& level = it->second;

            bool match = false;
            if (order->type == OrderType::MARKET) {
                match = true;
            } else {
                match = (price <= order->price);
            }
            if (!match) break;

            for (auto order_it = level.orders.begin(); order_it != level.orders.end();) {
                auto& counter_order = *order_it;
                Quantity trade_qty = std::min(
                    order->quantity - order->filled_quantity,
                    counter_order->quantity - counter_order->filled_quantity
                );
                if (trade_qty == 0) {
                    ++order_it;
                    continue;
                }
                Trade trade;
                trade.buy_order_id = order->id;
                trade.sell_order_id = counter_order->id;
                trade.price = price;
                trade.quantity = trade_qty;
                trade.timestamp = std::chrono::high_resolution_clock::now().time_since_epoch();
                trade.symbol = symbol_;
                trades.push_back(trade);

                order->filled_quantity += trade_qty;
                counter_order->filled_quantity += trade_qty;
                level.total_quantity -= trade_qty;

                total_trades_++;
                total_volume_ += trade_qty;

                if (trade_callback_) trade_callback_(trade);

                if (counter_order->filled_quantity == counter_order->quantity) {
                    counter_order->status = OrderStatus::FILLED;
                    order_it = level.orders.erase(order_it);
                } else {
                    counter_order->status = OrderStatus::PARTIAL;
                    ++order_it;
                }

                if (order_update_callback_) order_update_callback_(*counter_order);

                if (order->filled_quantity == order->quantity) break;
            }
            if (level.orders.empty()) {
                sell_orders_.erase(price);
            }
        }
    } else {
        // Match against buy orders
        while (order->filled_quantity < order->quantity && !buy_orders_.empty()) {
            auto it = buy_orders_.begin();
            auto price = it->first;
            auto& level = it->second;

            bool match = false;
            if (order->type == OrderType::MARKET) {
                match = true;
            } else {
                match = (price >= order->price);
            }
            if (!match) break;

            for (auto order_it = level.orders.begin(); order_it != level.orders.end();) {
                auto& counter_order = *order_it;
                Quantity trade_qty = std::min(
                    order->quantity - order->filled_quantity,
                    counter_order->quantity - counter_order->filled_quantity
                );
                if (trade_qty == 0) {
                    ++order_it;
                    continue;
                }
                Trade trade;
                trade.buy_order_id = counter_order->id;
                trade.sell_order_id = order->id;
                trade.price = price;
                trade.quantity = trade_qty;
                trade.timestamp = std::chrono::high_resolution_clock::now().time_since_epoch();
                trade.symbol = symbol_;
                trades.push_back(trade);

                order->filled_quantity += trade_qty;
                counter_order->filled_quantity += trade_qty;
                level.total_quantity -= trade_qty;

                total_trades_++;
                total_volume_ += trade_qty;

                if (trade_callback_) trade_callback_(trade);

                if (counter_order->filled_quantity == counter_order->quantity) {
                    counter_order->status = OrderStatus::FILLED;
                    order_it = level.orders.erase(order_it);
                } else {
                    counter_order->status = OrderStatus::PARTIAL;
                    ++order_it;
                }

                if (order_update_callback_) order_update_callback_(*counter_order);

                if (order->filled_quantity == order->quantity) break;
            }
            if (level.orders.empty()) {
                buy_orders_.erase(price);
            }
        }
    }
    return trades;
}

void OrderBook::process_market_order(std::shared_ptr<Order> order) {
    auto trades = match_orders(order);
    if (order->filled_quantity < order->quantity) {
        order->status = OrderStatus::REJECTED;
    }
}

void OrderBook::process_limit_order(std::shared_ptr<Order> order) {
    std::unique_lock lock(order_book_mutex_);
    add_order_to_level(order);
}

void OrderBook::process_stop_order(std::shared_ptr<Order> order) {
    Price market_price = (order->side == OrderSide::BUY) ? get_best_ask() : get_best_bid();
    if (market_price == 0) {
        order->status = OrderStatus::REJECTED;
        return;
    }

    bool triggered = (order->side == OrderSide::BUY) ? (market_price >= order->stop_price)
                                                     : (market_price <= order->stop_price);

    if (triggered) {
        order->type = OrderType::MARKET;
        process_market_order(order);
    }
}

void OrderBook::process_stop_limit_order(std::shared_ptr<Order> order) {
    Price market_price = (order->side == OrderSide::BUY) ? get_best_ask() : get_best_bid();
    if (market_price == 0) {
        order->status = OrderStatus::REJECTED;
        return;
    }

    bool triggered = (order->side == OrderSide::BUY) ? (market_price >= order->stop_price)
                                                     : (market_price <= order->stop_price);

    if (triggered) {
        order->type = OrderType::LIMIT;
        auto trades = match_orders(order);
        if (order->quantity > order->filled_quantity) {
            process_limit_order(order);
        }
    }
}

void OrderBook::add_order_to_level(std::shared_ptr<Order> order) {
    if (order->side == OrderSide::BUY) {
        auto& level = buy_orders_[order->price];
        if (level.price == 0) level.price = order->price;
        level.orders.push_back(order);
        level.total_quantity += (order->quantity - order->filled_quantity);
    } else {
        auto& level = sell_orders_[order->price];
        if (level.price == 0) level.price = order->price;
        level.orders.push_back(order);
        level.total_quantity += (order->quantity - order->filled_quantity);
    }
}

void OrderBook::remove_order_from_level(std::shared_ptr<Order> order) {
    if (order->side == OrderSide::BUY) {
        auto it = buy_orders_.find(order->price);
        if (it == buy_orders_.end()) return;
        auto& level = it->second;
        auto pos = std::find(level.orders.begin(), level.orders.end(), order);
        if (pos != level.orders.end()) {
            level.total_quantity -= ((*pos)->quantity - (*pos)->filled_quantity);
            level.orders.erase(pos);
        }
        if (level.orders.empty()) {
            buy_orders_.erase(it);
        }
    } else {
        auto it = sell_orders_.find(order->price);
        if (it == sell_orders_.end()) return;
        auto& level = it->second;
        auto pos = std::find(level.orders.begin(), level.orders.end(), order);
        if (pos != level.orders.end()) {
            level.total_quantity -= ((*pos)->quantity - (*pos)->filled_quantity);
            level.orders.erase(pos);
        }
        if (level.orders.empty()) {
            sell_orders_.erase(it);
        }
    }
}

bool OrderBook::cancel_order(OrderId order_id) {
    std::cout << "[LOG] OrderBook::cancel_order ENTER id=" << order_id << std::endl;
    std::shared_ptr<Order> order;
    {
        std::shared_lock lock(orders_mutex_);
        auto it = orders_by_id_.find(order_id);
        if (it == orders_by_id_.end()) {
            std::cout << "[LOG] OrderBook::cancel_order NOT FOUND id=" << order_id << std::endl;
            return false;
        }
        order = it->second;
    }

    if (order->status == OrderStatus::FILLED || order->status == OrderStatus::CANCELLED) {
        std::cout << "[LOG] OrderBook::cancel_order ALREADY FILLED/CANCELLED id=" << order_id << std::endl;
        return false;
    }

    order->status = OrderStatus::CANCELLED;

    if (order->type == OrderType::LIMIT) {
        std::unique_lock lock(order_book_mutex_);
        remove_order_from_level(order);
    }

    // Remove from orders_by_id_ so get_order returns nullptr
    {
        std::unique_lock lock(orders_mutex_);
        orders_by_id_.erase(order_id);
    }

    if (order_update_callback_) order_update_callback_(*order);
    std::cout << "[LOG] OrderBook::cancel_order EXIT id=" << order_id << std::endl;
    return true;
}

bool OrderBook::modify_order(OrderId order_id, Price new_price, Quantity new_quantity) {
    std::shared_ptr<Order> order;
    {
        std::shared_lock lock(orders_mutex_);
        auto it = orders_by_id_.find(order_id);
        if (it == orders_by_id_.end()) return false;
        order = it->second;
    }
    if (order->status == OrderStatus::FILLED || order->status == OrderStatus::CANCELLED) return false;
    if (order->filled_quantity >= order->quantity) return false;

    // In-place modify if reducing price/quantity
    bool can_modify_in_place = (new_quantity <= order->quantity && new_price == order->price);
    if (can_modify_in_place) {
        order->quantity = new_quantity;
        if (order_update_callback_) order_update_callback_(*order);
        return true;
    }
    // If price or quantity increases, cancel and re-add
    cancel_order(order_id);
    auto new_order = std::make_shared<Order>(
        order->id, order->symbol, order->side, order->type,
        new_price, new_quantity, order->user_id
    );
    add_order(new_order);
    return true;
}

// --- Metrics ---
double OrderBook::average_spread(size_t depth) const {
    std::shared_lock lock(order_book_mutex_);
    double total_spread = 0;
    size_t count = 0;
    auto bids = get_bid_levels(depth);
    auto asks = get_ask_levels(depth);
    size_t n = std::min(bids.size(), asks.size());
    for (size_t i = 0; i < n; ++i) {
        total_spread += static_cast<double>(asks[i].price) - static_cast<double>(bids[i].price);
        ++count;
    }
    return count ? total_spread / count : 0.0;
}

double OrderBook::order_to_trade_ratio() const {
    std::shared_lock lock(order_book_mutex_);
    return total_trades_ ? static_cast<double>(total_orders_) / total_trades_ : 0.0;
}

double OrderBook::cancellation_rate() const {
    std::shared_lock lock(order_book_mutex_);
    // This is a simple estimate: cancelled orders / total orders
    return total_orders_ ? static_cast<double>(total_orders_ - get_order_count()) / total_orders_ : 0.0;
}

Price OrderBook::get_best_bid() const {
    std::shared_lock lock(order_book_mutex_);
    return buy_orders_.empty() ? 0 : buy_orders_.begin()->first;
}

Price OrderBook::get_best_ask() const {
    std::shared_lock lock(order_book_mutex_);
    return sell_orders_.empty() ? 0 : sell_orders_.begin()->first;
}

Price OrderBook::get_spread() const {
    Price bid = get_best_bid();
    Price ask = get_best_ask();
    return (bid == 0 || ask == 0) ? 0 : ask - bid;
}

Quantity OrderBook::get_bid_depth(Price price) const {
    std::shared_lock lock(order_book_mutex_);
    Quantity total = 0;
    for (const auto& [p, level] : buy_orders_) {
        if (p >= price) total += level.total_quantity;
    }
    return total;
}

Quantity OrderBook::get_ask_depth(Price price) const {
    std::shared_lock lock(order_book_mutex_);
    Quantity total = 0;
    for (const auto& [p, level] : sell_orders_) {
        if (p <= price) total += level.total_quantity;
    }
    return total;
}

std::vector<OrderBookLevel> OrderBook::get_bid_levels(size_t depth) const {
    std::shared_lock lock(order_book_mutex_);
    std::vector<OrderBookLevel> result;
    for (const auto& [_, level] : buy_orders_) {
        if (result.size() >= depth) break;
        result.push_back(level);
    }
    return result;
}

std::vector<OrderBookLevel> OrderBook::get_ask_levels(size_t depth) const {
    std::shared_lock lock(order_book_mutex_);
    std::vector<OrderBookLevel> result;
    for (const auto& [_, level] : sell_orders_) {
        if (result.size() >= depth) break;
        result.push_back(level);
    }
    return result;
}

std::shared_ptr<Order> OrderBook::get_order(OrderId order_id) const {
    std::shared_lock lock(orders_mutex_);
    auto it = orders_by_id_.find(order_id);
    return (it != orders_by_id_.end()) ? it->second : nullptr;
}

void OrderBook::clear() {
    std::unique_lock lock1(order_book_mutex_);
    std::unique_lock lock2(orders_mutex_);
    buy_orders_.clear();
    sell_orders_.clear();
    orders_by_id_.clear();
    total_orders_ = total_trades_ = 0;
    total_volume_ = 0;
}

bool OrderBook::is_empty() const {
    std::shared_lock lock(order_book_mutex_);
    return buy_orders_.empty() && sell_orders_.empty();
}

size_t OrderBook::get_order_count() const {
    std::shared_lock lock(orders_mutex_);
    return orders_by_id_.size();
}

void OrderBook::cancel_expired_orders() {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::vector<OrderId> to_cancel;
    {
        std::shared_lock lock(orders_mutex_);
        for (const auto& [id, order] : orders_by_id_) {
            if (order->expiry > 0 && order->expiry <= now && order->status == OrderStatus::NEW) {
                to_cancel.push_back(id);
            }
        }
    }
    for (const auto& id : to_cancel) {
        cancel_order(id);
    }
}

}  // namespace orderbook
