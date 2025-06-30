#ifndef ORDERBOOK_ORDER_HPP
#define ORDERBOOK_ORDER_HPP

#include "utils.hpp"
#include <string>
#include <chrono>
#include <memory>

// Enum for order type
enum class OrderType 
{
    MARKET,
    LIMIT,
    STOP,
    STOP_LIMIT
};

// Enum for order side
enum class OrderSide 
{
    BUY,
    SELL
};

// Enum for order status
enum class OrderStatus 
{
    NEW,
    PARTIAL,
    FILLED,
    CANCELLED,
    REJECTED
};

// Order class
struct Order 
{
    orderbook::OrderId id;
    std::string symbol;
    OrderSide side;
    OrderType type;
    orderbook::Price price;         // For LIMIT/STOP_LIMIT orders
    orderbook::Price stop_price;    // For STOP/STOP_LIMIT orders
    orderbook::Quantity quantity;
    orderbook::Quantity filled_quantity = 0;
    OrderStatus status = OrderStatus::NEW;
    orderbook::UserId user_id;
    std::chrono::high_resolution_clock::time_point timestamp;
    // --- New fields for expiry and TIF ---
    int64_t expiry = 0; // Unix timestamp in seconds, 0 = no expiry (GTC)
    std::string tif = "GTC"; // Time-in-force: GTC, IOC, FOK, etc.

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

// Trade struct
struct Trade 
{
    orderbook::OrderId buy_order_id;
    orderbook::OrderId sell_order_id;
    std::string symbol;
    orderbook::Price price;
    orderbook::Quantity quantity;
    std::chrono::high_resolution_clock::duration timestamp;
};

#endif // ORDERBOOK_ORDER_HPP
