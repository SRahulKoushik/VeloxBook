#pragma once
#include <drogon/WebSocketController.h>
#include "matching_engine.hpp"
#include <set>
#include <mutex>

class OrderBookWebSocket : public drogon::WebSocketController<OrderBookWebSocket> 
{
public:
    static constexpr bool isAutoCreation = false;
    
    OrderBookWebSocket();
    OrderBookWebSocket(orderbook::MatchingEngine* engine);
    void handleNewMessage(const drogon::WebSocketConnectionPtr &wsConn,
                         std::string &&message,
                         const drogon::WebSocketMessageType &type) override;
    void handleNewConnection(const drogon::HttpRequestPtr &req,
                            const drogon::WebSocketConnectionPtr &wsConn) override;
    void handleConnectionClosed(const drogon::WebSocketConnectionPtr &wsConn) override;
    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/ws/orderbook", drogon::Get);
    WS_PATH_LIST_END
    // Broadcast helpers
    void broadcastOrderBook(const std::string& symbol);
    void broadcastTrade(const std::string& symbol, const std::string& tradeJson);
private:
    orderbook::MatchingEngine* engine_;
    std::set<drogon::WebSocketConnectionPtr> clients_;
    std::mutex clients_mutex_;
};
