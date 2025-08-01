#include "OrderBookWebSocket.h"
#include "matching_engine.hpp"
#include <json/json.h>
#include <drogon/WebSocketConnection.h>
#include <drogon/HttpRequest.h>
#include <sstream>

using namespace orderbook;

OrderBookWebSocket::OrderBookWebSocket() : engine_(nullptr) {}
OrderBookWebSocket::OrderBookWebSocket(orderbook::MatchingEngine* engine)
    : engine_(engine) {}

void OrderBookWebSocket::handleNewConnection(const drogon::HttpRequestPtr &req,
                                             const drogon::WebSocketConnectionPtr &wsConn) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_.insert(wsConn);
}

void OrderBookWebSocket::handleConnectionClosed(const drogon::WebSocketConnectionPtr &wsConn) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_.erase(wsConn);
}

void OrderBookWebSocket::handleNewMessage(const drogon::WebSocketConnectionPtr &wsConn,
                                          std::string &&message,
                                          const drogon::WebSocketMessageType &type) {
    // For now, just echo or ignore. You can implement subscription/filter logic here.
    // wsConn->send(message);
}

void OrderBookWebSocket::broadcastOrderBook(const std::string& symbol) {
    if (!engine_) return;
    auto bids = engine_->get_bid_levels(symbol, 20);
    auto asks = engine_->get_ask_levels(symbol, 20);
    Json::Value msg;
    msg["type"] = "orderbook";
    msg["symbol"] = symbol;
    msg["bids"] = Json::Value(Json::arrayValue);
    msg["asks"] = Json::Value(Json::arrayValue);
    for (const auto& b : bids) {
        Json::Value bid;
        bid["price"] = b.price;
        bid["quantity"] = b.total_quantity;
        msg["bids"].append(bid);
    }
    for (const auto& a : asks) {
        Json::Value ask;
        ask["price"] = a.price;
        ask["quantity"] = a.total_quantity;
        msg["asks"].append(ask);
    }
    Json::StreamWriterBuilder wbuilder;
    std::string payload = Json::writeString(wbuilder, msg);
    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (const auto& c : clients_) if (c->connected()) c->send(payload);
}

void OrderBookWebSocket::broadcastTrade(const std::string& symbol, const std::string& tradeJson) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (const auto& c : clients_) if (c->connected()) c->send(tradeJson);
}
