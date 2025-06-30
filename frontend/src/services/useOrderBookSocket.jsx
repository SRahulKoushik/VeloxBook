import React from 'react';
import { useState, useEffect, useRef } from 'react';

const WS_URL = (import.meta.env.VITE_WS_URL || 'ws://localhost:18080') + '/ws/orderbook';

export function useOrderBookSocket(symbol = 'BTCUSD') {
  const [orderBook, setOrderBook] = useState({ bids: [], asks: [] });
  const [trades, setTrades] = useState([]);
  const [isConnected, setIsConnected] = useState(false);
  const [error, setError] = useState(null);
  const wsRef = useRef(null);

  useEffect(() => {
    const connectWebSocket = () => {
      try {
        const ws = new WebSocket(WS_URL);
        wsRef.current = ws;

        ws.onopen = () => {
          console.log('WebSocket connected');
          setIsConnected(true);
          setError(null);
          
          // Subscribe to order book updates for the symbol
          ws.send(JSON.stringify({
            type: 'subscribe',
            symbol: symbol
          }));
        };

        ws.onmessage = (event) => {
          try {
            const data = JSON.parse(event.data);
            
            switch (data.type) {
              case 'orderbook':
                setOrderBook(data.data);
                break;
              case 'trade':
                setTrades(prev => [data.data, ...prev.slice(0, 99)]); // Keep last 100 trades
                break;
              default:
                console.log('Unknown message type:', data.type);
            }
          } catch (err) {
            console.error('Failed to parse WebSocket message:', err);
          }
        };

        ws.onclose = () => {
          console.log('WebSocket disconnected');
          setIsConnected(false);
          // Attempt to reconnect after 3 seconds
          setTimeout(connectWebSocket, 3000);
        };

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

    connectWebSocket();

    return () => {
      if (wsRef.current) {
        wsRef.current.close();
      }
    };
  }, [symbol]);

  const sendMessage = (message) => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify(message));
    }
  };

  return {
    orderBook,
    trades,
    isConnected,
    error,
    sendMessage
  };
}