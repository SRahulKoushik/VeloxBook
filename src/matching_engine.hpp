#ifndef ORDERBOOK_MATCHING_ENGINE_HPP
#define ORDERBOOK_MATCHING_ENGINE_HPP

#include "order.hpp"
#include "order_book.hpp"
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace orderbook {

struct EngineStats {
    size_t total_orders = 0;
    size_t total_trades = 0;
    Quantity total_volume = 0;
};

/**
 * @brief The MatchingEngine class coordinates multiple order books and provides a thread-safe API for order management.
 */
class MatchingEngine {
public:
    /**
     * @brief Construct a new Matching Engine object
     */
    MatchingEngine();

    /**
     * @brief Add a new order to the engine
     * @param order The order to add
     * @return Vector of trades resulting from the order
     */
    std::vector<Trade> add_order(std::shared_ptr<Order> order);

    /**
     * @brief Cancel an existing order by ID
     * @param order_id The order ID
     * @return True if cancelled, false otherwise
     */
    bool cancel_order(OrderId order_id);

    /**
     * @brief Modify an existing order
     * @param order_id The order ID
     * @param new_price The new price
     * @param new_quantity The new quantity
     * @return True if modified, false otherwise
     */
    bool modify_order(OrderId order_id, Price new_price, Quantity new_quantity);

    /**
     * @brief Get an order by its ID
     * @param order_id The order ID
     * @return Shared pointer to the order, or nullptr if not found
     */
    std::shared_ptr<Order> get_order(OrderId order_id) const;

    /**
     * @brief Get best bid for a symbol
     * @param symbol The symbol
     * @return Best bid price, or 0 if not available
     */
    Price get_best_bid(const std::string& symbol) const;

    /**
     * @brief Get best ask for a symbol
     * @param symbol The symbol
     * @return Best ask price, or 0 if not available
     */
    Price get_best_ask(const std::string& symbol) const;

    /**
     * @brief Get spread for a symbol
     * @param symbol The symbol
     * @return Spread, or 0 if not available
     */
    Price get_spread(const std::string& symbol) const;

    /**
     * @brief Get bid levels for a symbol
     * @param symbol The symbol
     * @param depth The number of levels
     * @return Vector of bid levels
     */
    std::vector<OrderBookLevel> get_bid_levels(const std::string& symbol, size_t depth) const;

    /**
     * @brief Get ask levels for a symbol
     * @param symbol The symbol
     * @param depth The number of levels
     * @return Vector of ask levels
     */
    std::vector<OrderBookLevel> get_ask_levels(const std::string& symbol, size_t depth) const;

    /**
     * @brief Get total number of active orders
     * @return Number of active orders
     */
    size_t get_order_count() const;

    /**
     * @brief Get all orders in the engine
     * @return Vector of all orders
     */
    std::vector<std::shared_ptr<Order>> get_all_orders() const;

    /**
     * @brief Get all orders for a specific user
     * @param user_id The user ID
     * @return Vector of user's orders
     */
    std::vector<std::shared_ptr<Order>> get_user_orders(const std::string& user_id) const;

    /**
     * @brief Get all trades for a specific user (MUST be implemented in .cpp)
     * @param user_id The user ID
     * @return Vector of user's trades
     */
    std::vector<Trade> get_user_trades(const std::string& user_id) const;

    /**
     * @brief Reset engine (clears all books)
     */
    void clear();

    /**
     * @brief Get engine statistics
     * @return EngineStats struct containing statistics
     */
    EngineStats get_stats() const;

    /**
     * @brief Cancel all expired orders in all books
     */
    void cancel_expired_orders();

    /**
     * @brief Add a trade to the trade history
     * @param trade The trade to add
     */
    void add_trade_history(const Trade& trade);

    // Engine-wide callbacks
    std::function<void(const Trade&)> on_trade;
    std::function<void(const Order&)> on_order_update;

private:
    mutable std::shared_mutex engine_mutex_;
    std::map<std::string, OrderBook> order_books_;
    std::unordered_map<OrderId, std::string> order_id_to_symbol_;
    EngineStats stats_;
    std::vector<Trade> trade_history_;
};

} // namespace orderbook

#endif // ORDERBOOK_MATCHING_ENGINE_HPP
