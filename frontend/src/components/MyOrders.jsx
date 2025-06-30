import React from 'react';
import { useEffect, useState } from 'react';
import { getMyOrders, cancelOrder } from '../services/api';
import { useAuth } from '../services/auth.jsx';
import toast from 'react-hot-toast';

function SkeletonRow() {
  return (
    <tr>
      <td className="animate-pulse bg-gray-200 h-4 w-20 rounded" />
      <td className="animate-pulse bg-gray-200 h-4 w-16 rounded" />
      <td className="animate-pulse bg-gray-200 h-4 w-16 rounded" />
      <td className="animate-pulse bg-gray-200 h-4 w-16 rounded" />
      <td className="animate-pulse bg-gray-200 h-4 w-12 rounded" />
    </tr>
  );
}

export default function MyOrders() {
  const { user } = useAuth();
  const [orders, setOrders] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);

  const userId = user?.username;

  function fetchOrders() {
    if (!userId) {
      setOrders([]);
      setLoading(false);
      return;
    }
    setLoading(true);
    getMyOrders(userId)
      .then(data => {
        setOrders(data.orders || data || []);
        setLoading(false);
      })
      .catch(err => {
        setError(err.message);
        setLoading(false);
      });
  }

  useEffect(() => {
    fetchOrders();
    // Listen for custom 'order-placed' event to refresh orders
    function handleOrderPlaced() {
      fetchOrders();
    }
    window.addEventListener('order-placed', handleOrderPlaced);
    return () => {
      window.removeEventListener('order-placed', handleOrderPlaced);
    };
    // eslint-disable-next-line
  }, [userId]);

  async function handleCancel(id) {
    try {
      await cancelOrder(id);
      toast.success('Order cancelled successfully');
      fetchOrders();
    } catch (err) {
      toast.error('Cancel failed: ' + err.message);
    }
  }

  const getStatusColor = (status) => {
    switch (status?.toLowerCase()) {
      case 'open': return 'text-green-600';
      case 'filled': return 'text-blue-600';
      case 'cancelled': return 'text-red-600';
      case 'partial': return 'text-yellow-600';
      default: return 'text-gray-600';
    }
  };

  if (!user) return (
    <div className="bg-white rounded-lg shadow p-4">
      <h2 className="text-lg sm:text-xl font-semibold mb-4 text-gray-800">My Orders</h2>
      <div className="text-center text-gray-500 py-8">
        <p>Please log in to view your orders.</p>
      </div>
    </div>
  );

  if (loading) return (
    <div className="bg-white rounded-lg shadow p-4">
      <h2 className="text-lg sm:text-xl font-semibold mb-4 text-gray-800">My Orders</h2>
      <div className="overflow-x-auto">
        <table className="w-full text-xs sm:text-sm">
          <thead>
            <tr className="border-b border-gray-200">
              <th className="text-left py-2 px-2">ID</th>
              <th className="text-left py-2 px-2">Symbol</th>
              <th className="text-left py-2 px-2">Side</th>
              <th className="text-right py-2 px-2">Price</th>
              <th className="text-right py-2 px-2">Qty</th>
              <th className="text-center py-2 px-2">Status</th>
              <th className="text-center py-2 px-2">Action</th>
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

  return (
    <div className="bg-white rounded-lg shadow p-4">
      <h2 className="text-lg sm:text-xl font-semibold mb-4 text-gray-800">My Orders</h2>
      
      {orders.length === 0 ? (
        <div className="text-center text-gray-500 py-8">
          <p>No orders found</p>
        </div>
      ) : (
        <div className="overflow-x-auto">
          <table className="w-full text-xs sm:text-sm">
            <thead>
              <tr className="border-b border-gray-200">
                <th className="text-left py-2 px-2">ID</th>
                <th className="text-left py-2 px-2">Symbol</th>
                <th className="text-left py-2 px-2">Side</th>
                <th className="text-right py-2 px-2">Price</th>
                <th className="text-right py-2 px-2">Qty</th>
                <th className="text-center py-2 px-2">Status</th>
                <th className="text-center py-2 px-2">Action</th>
              </tr>
            </thead>
            <tbody>
              {orders.map(order => (
                <tr key={order.id} className="border-b border-gray-100 hover:bg-gray-50">
                  <td className="py-2 px-2 font-mono text-xs">
                    {order.id?.slice(0, 8)}...
                  </td>
                  <td className="py-2 px-2 font-medium">
                    {order.symbol}
                  </td>
                  <td className="py-2 px-2">
                    <span className={`px-2 py-1 rounded text-xs font-medium ${
                      order.side === 'buy' 
                        ? 'bg-green-100 text-green-800' 
                        : 'bg-red-100 text-red-800'
                    }`}>
                      {order.side?.toUpperCase()}
                    </span>
                  </td>
                  <td className="py-2 px-2 text-right font-mono">
                    ${order.price?.toFixed(2) || order.price}
                  </td>
                  <td className="py-2 px-2 text-right font-mono">
                    {order.quantity}
                  </td>
                  <td className="py-2 px-2 text-center">
                    <span className={`text-xs font-medium ${getStatusColor(order.status)}`}>
                      {order.status?.toUpperCase() || 'UNKNOWN'}
                    </span>
                  </td>
                  <td className="py-2 px-2 text-center">
                    {(order.status === 'open' || order.status === 'partial') && (
                      <button
                        onClick={() => handleCancel(order.id)}
                        className="text-xs bg-red-600 text-white px-2 py-1 rounded hover:bg-red-700 transition-colors"
                      >
                        Cancel
                      </button>
                    )}
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