import React from 'react';
import { useEffect, useState } from 'react';
import { getTradeHistory } from '../services/api';
import { useAuth } from '../services/auth.jsx';

function SkeletonRow() {
  return (
    <tr>
      <td className="animate-pulse bg-gray-200 h-4 w-20 rounded" />
      <td className="animate-pulse bg-gray-200 h-4 w-16 rounded" />
      <td className="animate-pulse bg-gray-200 h-4 w-12 rounded" />
    </tr>
  );
}

export default function TradeHistory({ trades = [] }) {
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);
  const [history, setHistory] = useState([]);
  const { user } = useAuth();

  useEffect(() => {
    if (!user) {
      setLoading(false);
      setHistory([]);
      return;
    }
    getTradeHistory(user.username)
      .then(data => {
        setHistory(data.trades || data || []);
        setLoading(false);
      })
      .catch(err => {
        setError(err.message);
        setLoading(false);
      });
  }, [user]);

  if (!user) return (
    <div className="bg-white rounded-lg shadow p-4">
      <h2 className="text-lg sm:text-xl font-semibold mb-4 text-gray-800">Trade History</h2>
      <div className="text-center text-gray-500 py-8">
        <p>Please log in to view your trade history.</p>
      </div>
    </div>
  );

  if (loading) return (
    <div className="bg-white rounded-lg shadow p-4">
      <h2 className="text-lg sm:text-xl font-semibold mb-4 text-gray-800">Trade History</h2>
      <div className="overflow-x-auto">
        <table className="w-full text-xs sm:text-sm">
          <thead>
            <tr className="border-b border-gray-200">
              <th className="text-left py-2 px-2">Time</th>
              <th className="text-right py-2 px-2">Price</th>
              <th className="text-right py-2 px-2">Qty</th>
            </tr>
          </thead>
          <tbody>
            {Array.from({ length: 8 }).map((_, i) => <SkeletonRow key={i} />)}
          </tbody>
        </table>
      </div>
    </div>
  );

  if (error) return (
    <div className="bg-white rounded-lg shadow p-4">
      <div className="text-red-600 text-center">Error: {error}</div>
    </div>
  );

  const formatTime = (timestamp) => {
    if (!timestamp) return '-';
    const date = new Date(timestamp);
    return date.toLocaleTimeString();
  };

  return (
    <div className="bg-white rounded-lg shadow p-4">
      <h2 className="text-lg sm:text-xl font-semibold mb-4 text-gray-800">Trade History</h2>
      
      {history.length === 0 ? (
        <div className="text-center text-gray-500 py-8">
          <p>No trades yet</p>
        </div>
      ) : (
        <div className="overflow-x-auto">
          <table className="w-full text-xs sm:text-sm">
            <thead>
              <tr className="border-b border-gray-200">
                <th className="text-left py-2 px-2">Time</th>
                <th className="text-right py-2 px-2">Price</th>
                <th className="text-right py-2 px-2">Qty</th>
              </tr>
            </thead>
            <tbody>
              {history.slice(0, 20).map((trade, i) => (
                <tr key={i} className="border-b border-gray-100 hover:bg-gray-50">
                  <td className="py-2 px-2 text-gray-600">
                    {formatTime(trade.timestamp || trade.time)}
                  </td>
                  <td className="py-2 px-2 text-right font-mono">
                    ${trade.price?.toFixed(2) || trade.price}
                  </td>
                  <td className="py-2 px-2 text-right font-mono">
                    {trade.quantity}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
    </div>
  );
} 