import React from 'react';
import { useState, useEffect } from 'react';

const defaultPrices = [
  { symbol: 'BTCUSD', price: 42000, change: 2.5 },
  { symbol: 'ETHUSD', price: 3200, change: -1.2 },
  { symbol: 'DOGEUSD', price: 0.12, change: 5.8 },
];

export default function TickerTape() {
  const [prices, setPrices] = useState(defaultPrices);

  useEffect(() => {
    // Simulate real-time price updates
    const interval = setInterval(() => {
      setPrices(prev => prev.map(price => ({
        ...price,
        price: price.price + (Math.random() - 0.5) * 100,
        change: price.change + (Math.random() - 0.5) * 2
      })));
    }, 5000);

    return () => clearInterval(interval);
  }, []);

  return (
    <div className="bg-gradient-to-r from-indigo-50 to-blue-50 border border-indigo-200 rounded-lg p-3 mb-4">
      <div className="overflow-x-auto">
        <div className="flex space-x-6 sm:space-x-8 lg:space-x-12 animate-pulse">
          {prices.map((item) => (
            <div key={item.symbol} className="flex-shrink-0">
              <div className="flex flex-col sm:flex-row sm:items-center sm:space-x-2">
                <span className="font-mono text-sm sm:text-base font-semibold text-gray-800">
                  {item.symbol}
                </span>
                <span className="font-mono text-sm sm:text-base font-bold text-indigo-700">
                  ${item.price.toFixed(2)}
                </span>
                <span className={`text-xs sm:text-sm font-medium ${
                  item.change >= 0 ? 'text-green-600' : 'text-red-600'
                }`}>
                  {item.change >= 0 ? '+' : ''}{item.change.toFixed(2)}%
                </span>
              </div>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
} 