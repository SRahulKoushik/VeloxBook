import React from 'react';
import { useState } from 'react';
import { placeOrder } from '../services/api';
import { useAuth } from '../services/auth.jsx';
import toast from 'react-hot-toast';

// Order placement form - this is where users actually trade
// I've made it simple but functional with proper validation
// The form changes color based on buy/sell to make it intuitive
export default function OrderForm() {
  const { user } = useAuth();
  const [loading, setLoading] = useState(false);
  const [form, setForm] = useState({
    symbol: 'BTCUSD',
    side: 'buy',
    type: 'limit',
    price: '',
    quantity: '',
    user_id: user?.username || 'user1'
  });

  // Update form state when user types
  function handleChange(e) {
    setForm({ ...form, [e.target.name]: e.target.value });
  }

  // Handle order submission
  // This is where the magic happens - sends order to the backend
  async function handleSubmit(e) {
    e.preventDefault();
    if (!user) {
      toast.error('You must be logged in to place an order');
      return;
    }
    if (!form.price || !form.quantity) {
      toast.error('Please fill in all required fields');
      return;
    }
    setLoading(true);
    try {
      const result = await placeOrder({
        ...form,
        user_id: user.username,
        price: parseInt(form.price, 10),
        quantity: parseInt(form.quantity, 10)
      });
      toast.success('Order placed successfully!');
      // Trigger order book refresh so users see their order immediately
      window.dispatchEvent(new Event('order-placed'));
      setForm({
        ...form,
        price: '',
        quantity: ''
      });
    } catch (err) {
      toast.error('Order failed: ' + err.message);
    } finally {
      setLoading(false);
    }
  }

  // Show login prompt if user isn't authenticated
  if (!user) return (
    <div className="bg-white rounded-lg shadow p-4 sm:p-6">
      <h2 className="text-lg sm:text-xl font-semibold mb-4 text-gray-800">Place Order</h2>
      <div className="text-center text-gray-500 py-8">
        <p>Please log in to place an order.</p>
      </div>
    </div>
  );

  return (
    <div className="bg-white rounded-lg shadow p-4 sm:p-6">
      <h2 className="text-lg sm:text-xl font-semibold mb-4 text-gray-800">Place Order</h2>
      
      <form onSubmit={handleSubmit} className="space-y-4">
        {/* Trading symbol - what you're buying/selling */}
        <div>
          <label className="block text-sm font-medium text-gray-700 mb-1">
            Symbol
          </label>
          <input
            name="symbol"
            value={form.symbol}
            onChange={handleChange}
            placeholder="Symbol (e.g. BTCUSD)"
            className="w-full px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500 text-sm sm:text-base"
            required
          />
        </div>

        {/* Buy/sell side and order type */}
        <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
          <div>
            <label className="block text-sm font-medium text-gray-700 mb-1">
              Side
            </label>
            <select
              name="side"
              value={form.side}
              onChange={handleChange}
              className="w-full px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500 text-sm sm:text-base"
            >
              <option value="buy">Buy</option>
              <option value="sell">Sell</option>
            </select>
          </div>
          
          <div>
            <label className="block text-sm font-medium text-gray-700 mb-1">
              Type
            </label>
            <select
              name="type"
              value={form.type}
              onChange={handleChange}
              className="w-full px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500 text-sm sm:text-base"
            >
              <option value="limit">Limit</option>
              <option value="market">Market</option>
            </select>
          </div>
        </div>

        {/* Price and quantity inputs */}
        <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
          <div>
            <label className="block text-sm font-medium text-gray-700 mb-1">
              Price
            </label>
            <input
              name="price"
              value={form.price}
              onChange={handleChange}
              placeholder="0.00"
              type="number"
              step="0.01"
              min="0"
              className="w-full px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500 text-sm sm:text-base"
              required
            />
          </div>
          
          <div>
            <label className="block text-sm font-medium text-gray-700 mb-1">
              Quantity
            </label>
            <input
              name="quantity"
              value={form.quantity}
              onChange={handleChange}
              placeholder="0"
              type="number"
              step="0.01"
              min="0"
              className="w-full px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500 text-sm sm:text-base"
              required
            />
          </div>
        </div>

        {/* Submit button - changes color based on buy/sell */}
        <button
          type="submit"
          disabled={loading}
          className={`w-full py-2 sm:py-3 px-4 border border-transparent text-sm sm:text-base font-medium rounded-md text-white transition-colors ${
            form.side === 'buy' 
              ? 'bg-green-600 hover:bg-green-700 focus:ring-green-500' 
              : 'bg-red-600 hover:bg-red-700 focus:ring-red-500'
          } focus:outline-none focus:ring-2 focus:ring-offset-2 disabled:opacity-50`}
        >
          {loading ? 'Placing Order...' : `${form.side === 'buy' ? 'Buy' : 'Sell'} ${form.symbol}`}
        </button>
      </form>
    </div>
  );
} 