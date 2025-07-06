#ifndef ORDERBOOK_ORDER_HPP
#define ORDERBOOK_ORDER_HPP

#include "utils.hpp"
#include <string>
#include <chrono>
#include <memory>

// Different types of orders you can place
enum class OrderType 
{
    MARKET,      // Execute immediately at best available price
    LIMIT,       // Only execute at specified price or better
    STOP,        // Market order that triggers at a specific price
    STOP_LIMIT   // Limit order that triggers at a specific price
};

// Whether you're buying or selling
enum class OrderSide 
{
    BUY,         // You want to buy (you're a bidder)
    SELL         // You want to sell (you're an asker)
};

// Current status of an order
enum class OrderStatus 
{
    NEW,         // Just placed, waiting to be processed
    PARTIAL,     // Partially filled, some quantity remaining
    FILLED,      // Completely filled
    CANCELLED,   // Cancelled by user or system
    REJECTED     // Rejected due to invalid parameters
};

// Represents a trading order
// 
// This is the core data structure that represents a user's intent to trade.
// When you place an order, it gets stored as one of these objects and
// processed by the matching engine.
struct Order 
{
    orderbook::OrderId id;          // Unique identifier for this order
    std::string symbol;             // What you're trading (e.g., "BTC-USD")
    OrderSide side;                 // Buy or sell
    OrderType type;                 // Market, limit, stop, etc.
    orderbook::Price price;         // Your limit price (0 for market orders)
    orderbook::Price stop_price;    // Trigger price for stop orders
    orderbook::Quantity quantity;   // How much you want to trade
    orderbook::Quantity filled_quantity = 0;  // How much has been filled so far
    OrderStatus status = OrderStatus::NEW;    // Current status
    orderbook::UserId user_id;      // Who placed this order
    std::chrono::high_resolution_clock::time_point timestamp;  // When it was placed
    
    // Time-in-Force and expiry settings
    int64_t expiry = 0;             // When this order expires (Unix timestamp, 0 = never)
    std::string tif = "GTC";        // Time-in-Force: GTC (Good Till Cancelled), IOC (Immediate or Cancel), FOK (Fill or Kill)

    // Constructor - creates a new order
    // 
    // Most parameters are self-explanatory. A few notes:
    // - stop_price is only used for STOP and STOP_LIMIT orders
    // - expiry of 0 means the order never expires (GTC)
    // - tif defaults to "GTC" (Good Till Cancelled)
    Order(orderbook::OrderId id_,
          const std::string& symbol_,
          OrderSide side_,
          OrderType type_,
          orderbook::Price price_,
          orderbook::Quantity quantity_,
          const orderbook::UserId& user_id_,
          orderbook::Price stop_price_ = 0,
          int64_t expiry_ = 0,
          const std::string& tif_ = "GTC")
        : id(std::move(id_)),
          symbol(symbol_),
          side(side_),
          type(type_),
          price(price_),
          stop_price(stop_price_),
          quantity(quantity_),
          user_id(user_id_),
          timestamp(std::chrono::high_resolution_clock::now()),
          expiry(expiry_),
          tif(tif_)
    {}
};

// Represents a completed trade
// 
// When two orders match (a buy meets a sell), a trade is created.
// This records who traded with whom, at what price, and how much.
struct Trade 
{
    orderbook::OrderId buy_order_id;   // ID of the buy order
    orderbook::OrderId sell_order_id;  // ID of the sell order
    std::string symbol;                // What was traded
    orderbook::Price price;            // Price the trade happened at
    orderbook::Quantity quantity;      // How much was traded
    std::chrono::high_resolution_clock::duration timestamp;  // When the trade happened
};

#endif // ORDERBOOK_ORDER_HPP
