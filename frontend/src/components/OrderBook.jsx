import React from 'react';
import { useEffect, useState } from 'react';
import { getOrderBook } from '../services/api';

// Loading skeleton for when data is being fetched
// Shows animated placeholders so the UI doesn't jump around
function SkeletonRow() {
  return (
    <tr>
      <td className="animate-pulse bg-gray-200 h-4 w-16 rounded" />
      <td className="animate-pulse bg-gray-200 h-4 w-12 rounded" />
    </tr>
  );
}

// The main order book component - shows live bid and ask prices
// This is probably the most important part of any trading interface
// I've made it responsive so it works well on mobile and desktop
export default function OrderBook({ orderBook = { bids: [], asks: [] } }) {
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);

  useEffect(() => {
    // Load initial data for fast UI, but will be replaced by WebSocket updates
    // This gives users something to look at immediately
    getOrderBook('BTCUSD')
      .then(() => {
        setLoading(false);
      })
      .catch(err => {
        setError(err.message);
        setLoading(false);
      });
    
    // Listen for custom 'order-placed' event to refresh order book
    // This ensures the order book updates when users place orders
    function handleOrderPlaced() {
      setLoading(true);
      getOrderBook('BTCUSD')
        .then(() => {
          setLoading(false);
        })
        .catch(err => {
          setError(err.message);
          setLoading(false);
        });
    }
    window.addEventListener('order-placed', handleOrderPlaced);
    return () => {
      window.removeEventListener('order-placed', handleOrderPlaced);
    };
  }, []);

  // Show error message if something goes wrong
  if (error) return (
    <div className="bg-white rounded-lg shadow p-4">
      <div className="text-red-600 text-center">Error: {error}</div>
    </div>
  );

  // Show loading skeleton while fetching data
  if (loading) return (
    <div className="bg-white rounded-lg shadow p-4">
      <h2 className="text-lg sm:text-xl font-semibold mb-4 text-gray-800">Order Book</h2>
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-4">
        <div>
          <h3 className="text-green-600 font-bold text-sm sm:text-base mb-2">Bids</h3>
          <div className="overflow-x-auto">
            <table className="w-full text-xs sm:text-sm">
              <thead>
                <tr className="border-b border-gray-200">
                  <th className="text-left py-2 px-2">Price</th>
                  <th className="text-right py-2 px-2">Qty</th>
                </tr>
              </thead>
              <tbody>
                {Array.from({ length: 8 }).map((_, i) => <SkeletonRow key={i} />)}
              </tbody>
            </table>
          </div>
        </div>
        <div>
          <h3 className="text-red-600 font-bold text-sm sm:text-base mb-2">Asks</h3>
          <div className="overflow-x-auto">
            <table className="w-full text-xs sm:text-sm">
              <thead>
                <tr className="border-b border-gray-200">
                  <th className="text-left py-2 px-2">Price</th>
                  <th className="text-right py-2 px-2">Qty</th>
                </tr>
              </thead>
              <tbody>
                {Array.from({ length: 8 }).map((_, i) => <SkeletonRow key={i} />)}
              </tbody>
            </table>
          </div>
        </div>
      </div>
    </div>
  );

  const { bids = [], asks = [] } = orderBook;

  return (
    <div className="bg-white rounded-lg shadow p-4">
      <h2 className="text-lg sm:text-xl font-semibold mb-4 text-gray-800">Order Book</h2>
      
      {/* Mobile Layout - Stacked vertically to save space */}
      <div className="block lg:hidden space-y-4">
        <div>
          <h3 className="text-green-600 font-bold text-sm mb-2">Bids</h3>
          <div className="overflow-x-auto">
            <table className="w-full text-xs">
              <thead>
                <tr className="border-b border-gray-200">
                  <th className="text-left py-2 px-2">Price</th>
                  <th className="text-right py-2 px-2">Qty</th>
                </tr>
              </thead>
              <tbody>
                {bids.slice(0, 5).map((bid, i) => (
                  <tr key={i} className="border-b border-gray-100">
                    <td className="py-2 px-2 text-green-600">{bid.price}</td>
                    <td className="py-2 px-2 text-right">{bid.quantity}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
        
        <div>
          <h3 className="text-red-600 font-bold text-sm mb-2">Asks</h3>
          <div className="overflow-x-auto">
            <table className="w-full text-xs">
              <thead>
                <tr className="border-b border-gray-200">
                  <th className="text-left py-2 px-2">Price</th>
                  <th className="text-right py-2 px-2">Qty</th>
                </tr>
              </thead>
              <tbody>
                {asks.slice(0, 5).map((ask, i) => (
                  <tr key={i} className="border-b border-gray-100">
                    <td className="py-2 px-2 text-red-600">{ask.price}</td>
                    <td className="py-2 px-2 text-right">{ask.quantity}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      </div>

      {/* Desktop Layout - Side by side for better comparison */}
      <div className="hidden lg:grid lg:grid-cols-2 gap-6">
        <div>
          <h3 className="text-green-600 font-bold text-base mb-3">Bids</h3>
          <div className="overflow-x-auto">
            <table className="w-full text-sm">
              <thead>
                <tr className="border-b border-gray-200">
                  <th className="text-left py-2 px-3">Price</th>
                  <th className="text-right py-2 px-3">Qty</th>
                </tr>
              </thead>
              <tbody>
                {bids.slice(0, 10).map((bid, i) => (
                  <tr key={i} className="border-b border-gray-100 hover:bg-gray-50">
                    <td className="py-2 px-3 text-green-600">{bid.price}</td>
                    <td className="py-2 px-3 text-right">{bid.quantity}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
        
        <div>
          <h3 className="text-red-600 font-bold text-base mb-3">Asks</h3>
          <div className="overflow-x-auto">
            <table className="w-full text-sm">
              <thead>
                <tr className="border-b border-gray-200">
                  <th className="text-left py-2 px-3">Price</th>
                  <th className="text-right py-2 px-3">Qty</th>
                </tr>
              </thead>
              <tbody>
                {asks.slice(0, 10).map((ask, i) => (
                  <tr key={i} className="border-b border-gray-100 hover:bg-gray-50">
                    <td className="py-2 px-3 text-red-600">{ask.price}</td>
                    <td className="py-2 px-3 text-right">{ask.quantity}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      </div>
    </div>
  );
} 