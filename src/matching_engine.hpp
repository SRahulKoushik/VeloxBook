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

// Statistics about the engine's performance
// Useful for monitoring and debugging
struct EngineStats {
    size_t total_orders = 0;
    size_t total_trades = 0;
    Quantity total_volume = 0;
};

/**
 * The heart of the trading platform - the matching engine
 * 
 * This class manages multiple order books (one per trading symbol) and handles
 * all the complex logic of matching buy and sell orders. It's thread-safe so
 * multiple users can place orders simultaneously without conflicts.
 * 
 * The engine automatically matches orders when a buy price meets or exceeds
 * a sell price, creating trades and updating the order book in real-time.
 */
class MatchingEngine {
public:
    /**
     * Create a new matching engine
     * 
     * The engine starts empty and you add order books for each symbol
     * you want to trade (like "BTC-USD", "ETH-USD", etc.)
     */
    MatchingEngine();

    /**
     * Add a new order to the engine
     * 
     * This is the main method you'll call to place orders. The engine will:
     * 1. Try to match the order against existing orders
     * 2. Create trades for any matches
     * 3. Add the remaining quantity to the order book
     * 4. Return a list of all trades that happened
     * 
     * @param order The order to add (buy or sell, market or limit)
     * @return List of trades that resulted from this order
     */
    std::vector<Trade> add_order(std::shared_ptr<Order> order);

    /**
     * Cancel an existing order
     * 
     * Removes the order from the book if it hasn't been filled yet.
     * Returns false if the order doesn't exist or is already filled.
     * 
     * @param order_id The unique ID of the order to cancel
     * @return True if cancelled successfully, false otherwise
     */
    bool cancel_order(OrderId order_id);

    /**
     * Modify an existing order's price and quantity
     * 
     * This is like cancelling the old order and placing a new one,
     * but it's atomic and preserves the original order ID.
     * 
     * @param order_id The order to modify
     * @param new_price The new price (0 for market orders)
     * @param new_quantity The new quantity
     * @return True if modified successfully, false if order not found
     */
    bool modify_order(OrderId order_id, Price new_price, Quantity new_quantity);

    /**
     * Get an order by its ID
     * 
     * Useful for checking order status or getting order details.
     * 
     * @param order_id The order ID to look up
     * @return Pointer to the order, or nullptr if not found
     */
    std::shared_ptr<Order> get_order(OrderId order_id) const;

    /**
     * Get the best bid price for a symbol
     * 
     * The best bid is the highest price someone is willing to pay.
     * Returns 0 if there are no bids for this symbol.
     * 
     * @param symbol The trading symbol (e.g., "BTC-USD")
     * @return Best bid price, or 0 if no bids
     */
    Price get_best_bid(const std::string& symbol) const;

    /**
     * Get the best ask price for a symbol
     * 
     * The best ask is the lowest price someone is willing to sell for.
     * Returns 0 if there are no asks for this symbol.
     * 
     * @param symbol The trading symbol (e.g., "BTC-USD")
     * @return Best ask price, or 0 if no asks
     */
    Price get_best_ask(const std::string& symbol) const;

    /**
     * Get the spread for a symbol
     * 
     * The spread is the difference between best ask and best bid.
     * A tight spread usually means high liquidity.
     * 
     * @param symbol The trading symbol
     * @return Spread (ask - bid), or 0 if either side is empty
     */
    Price get_spread(const std::string& symbol) const;

    /**
     * Get the top bid levels for a symbol
     * 
     * Returns the best N bid prices with their total quantities.
     * Useful for showing order book depth.
     * 
     * @param symbol The trading symbol
     * @param depth How many levels to return
     * @return List of bid levels (price + quantity)
     */
    std::vector<OrderBookLevel> get_bid_levels(const std::string& symbol, size_t depth) const;

    /**
     * Get the top ask levels for a symbol
     * 
     * Returns the best N ask prices with their total quantities.
     * Useful for showing order book depth.
     * 
     * @param symbol The trading symbol
     * @param depth How many levels to return
     * @return List of ask levels (price + quantity)
     */
    std::vector<OrderBookLevel> get_ask_levels(const std::string& symbol, size_t depth) const;

    /**
     * Get the total number of active orders
     * 
     * This counts all open orders across all symbols.
     * 
     * @return Total number of active orders
     */
    size_t get_order_count() const;

    /**
     * Get all orders in the engine
     * 
     * Returns every active order across all symbols.
     * Be careful with this on large order books - it could be slow.
     * 
     * @return List of all active orders
     */
    std::vector<std::shared_ptr<Order>> get_all_orders() const;

    /**
     * Get all orders for a specific user
     * 
     * Useful for showing a user their open orders.
     * 
     * @param user_id The user's ID
     * @return List of the user's active orders
     */
    std::vector<std::shared_ptr<Order>> get_user_orders(const std::string& user_id) const;

    /**
     * Get all trades for a specific user
     * 
     * Returns the user's trade history across all symbols.
     * This is implemented in the .cpp file.
     * 
     * @param user_id The user's ID
     * @return List of the user's trades
     */
    std::vector<Trade> get_user_trades(const std::string& user_id) const;

    /**
     * Clear all order books
     * 
     * Removes all orders and trades. Useful for testing or resetting.
     */
    void clear();

    /**
     * Get engine statistics
     * 
     * Returns counts of total orders, trades, and volume processed.
     * 
     * @return Statistics about the engine's activity
     */
    EngineStats get_stats() const;

    /**
     * Cancel all expired orders
     * 
     * Goes through all order books and cancels orders that have expired.
     * Call this periodically to clean up expired orders.
     */
    void cancel_expired_orders();

    /**
     * Add a trade to the history
     * 
     * Called internally when trades are executed.
     * 
     * @param trade The trade to record
     */
    void add_trade_history(const Trade& trade);

    // Callbacks for real-time updates
    // Set these to get notified when trades happen or orders change
    std::function<void(const Trade&)> on_trade;
    std::function<void(const Order&)> on_order_update;

private:
    // Thread safety - multiple readers, single writer
    mutable std::shared_mutex engine_mutex_;
    
    // One order book per trading symbol
    std::map<std::string, OrderBook> order_books_;
    
    // Quick lookup to find which symbol an order belongs to
    std::unordered_map<OrderId, std::string> order_id_to_symbol_;
    
    // Engine statistics
    EngineStats stats_;
    
    // History of all trades (for user queries)
    std::vector<Trade> trade_history_;
};

} // namespace orderbook

#endif // ORDERBOOK_MATCHING_ENGINE_HPP
