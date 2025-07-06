import React from 'react';
import { useState, useEffect, useRef } from 'react';

// WebSocket URL for real-time updates
// This connects to the backend's WebSocket endpoint for live order book data
const WS_URL = (import.meta.env.VITE_WS_URL || 'ws://localhost:18080') + '/ws/orderbook';

// Custom hook for WebSocket connection to the order book
// This is what makes the trading interface feel real-time
// Instead of polling the server every few seconds, we get instant updates
export function useOrderBookSocket(symbol = 'BTCUSD') {
  const [orderBook, setOrderBook] = useState({ bids: [], asks: [] });
  const [trades, setTrades] = useState([]);
  const [isConnected, setIsConnected] = useState(false);
  const [error, setError] = useState(null);
  const wsRef = useRef(null);

  useEffect(() => {
    // Function to establish WebSocket connection
    // I've included automatic reconnection so the connection stays alive
    const connectWebSocket = () => {
      try {
        const ws = new WebSocket(WS_URL);
        wsRef.current = ws;

        // Connection established successfully
        ws.onopen = () => {
          console.log('WebSocket connected');
          setIsConnected(true);
          setError(null);
          
          // Subscribe to order book updates for the specific symbol
          // This tells the server what data we want to receive
          ws.send(JSON.stringify({
            type: 'subscribe',
            symbol: symbol
          }));
        };

        // Handle incoming messages from the server
        // This is where we get real-time order book and trade updates
        ws.onmessage = (event) => {
          try {
            const data = JSON.parse(event.data);
            
            switch (data.type) {
              case 'orderbook':
                // Update the order book with latest bids and asks
                setOrderBook(data.data);
                break;
              case 'trade':
                // Add new trade to the top of the list, keep last 100
                // This prevents the trade history from growing too large
                setTrades(prev => [data.data, ...prev.slice(0, 99)]);
                break;
              default:
                console.log('Unknown message type:', data.type);
            }
          } catch (err) {
            console.error('Failed to parse WebSocket message:', err);
          }
        };

        // Handle connection closure
        // Automatically try to reconnect after 3 seconds
        ws.onclose = () => {
          console.log('WebSocket disconnected');
          setIsConnected(false);
          setTimeout(connectWebSocket, 3000);
        };

        // Handle connection errors
        ws.onerror = (error) => {
          console.error('WebSocket error:', error);
          setError('WebSocket connection failed');
          setIsConnected(false);
        };

      } catch (err) {
        console.error('Failed to create WebSocket:', err);
        setError('Failed to connect to WebSocket');
        setIsConnected(false);
      }
    };

    // Start the WebSocket connection
    connectWebSocket();

    // Cleanup function - close connection when component unmounts
    return () => {
      if (wsRef.current) {
        wsRef.current.close();
      }
    };
  }, [symbol]);

  // Helper function to send messages to the server
  // Useful for subscribing to different symbols or sending commands
  const sendMessage = (message) => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify(message));
    }
  };

  // Return everything the components need
  return {
    orderBook,      // Current order book data
    trades,         // Recent trades
    isConnected,    // Connection status
    error,          // Any connection errors
    sendMessage     // Function to send messages
  };
}