import React from 'react';
import { useState } from 'react';
import { useAuth } from '../services/auth.jsx';
import OrderBook from '../components/OrderBook';
import TradeHistory from '../components/TradeHistory';
import OrderForm from '../components/OrderForm';
import MyOrders from '../components/MyOrders';
import { useOrderBookSocket } from '../services/useOrderBookSocket.jsx';

export default function Dashboard() {
  const { user } = useAuth();
  const [activeTab, setActiveTab] = useState('orderbook');
  const { orderBook, trades, isConnected } = useOrderBookSocket('BTCUSD');

  const tabs = [
    { id: 'orderbook', name: 'Order Book', component: <OrderBook orderBook={orderBook} /> },
    { id: 'trades', name: 'Trade History', component: <TradeHistory trades={trades} /> },
    { id: 'place-order', name: 'Place Order', component: <OrderForm /> },
    { id: 'my-orders', name: 'My Orders', component: <MyOrders /> },
  ];

  return (
    <div className="space-y-4 sm:space-y-6">
      {/* Header */}
      <div className="bg-white shadow rounded-lg p-4 sm:p-6">
        <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between space-y-4 sm:space-y-0">
          <div>
            <h1 className="text-xl sm:text-2xl font-bold text-gray-900">Trading Dashboard</h1>
            <p className="text-sm sm:text-base text-gray-600">Welcome back, {user?.username || 'User'}!</p>
          </div>
          <div className="flex items-center space-x-4">
            <div className={`flex items-center space-x-2 ${isConnected ? 'text-green-600' : 'text-red-600'}`}>
              <div className={`w-2 h-2 rounded-full ${isConnected ? 'bg-green-500' : 'bg-red-500'}`}></div>
              <span className="text-xs sm:text-sm font-medium">
                {isConnected ? 'Connected' : 'Disconnected'}
              </span>
            </div>
          </div>
        </div>
      </div>

      {/* Tabs */}
      <div className="bg-white shadow rounded-lg">
        {/* Mobile Tab Selector */}
        <div className="sm:hidden p-4 border-b border-gray-200">
          <select
            value={activeTab}
            onChange={(e) => setActiveTab(e.target.value)}
            className="w-full p-2 border border-gray-300 rounded-md focus:outline-none focus:ring-2 focus:ring-indigo-500"
          >
            {tabs.map((tab) => (
              <option key={tab.id} value={tab.id}>
                {tab.name}
              </option>
            ))}
          </select>
        </div>

        {/* Desktop Tabs */}
        <div className="hidden sm:block border-b border-gray-200">
          <nav className="-mb-px flex space-x-8 px-6">
            {tabs.map((tab) => (
              <button
                key={tab.id}
                onClick={() => setActiveTab(tab.id)}
                className={`py-4 px-1 border-b-2 font-medium text-sm ${
                  activeTab === tab.id
                    ? 'border-indigo-500 text-indigo-600'
                    : 'border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300'
                }`}
              >
                {tab.name}
              </button>
            ))}
          </nav>
        </div>

        {/* Tab Content */}
        <div className="p-4 sm:p-6">
          {tabs.find(tab => tab.id === activeTab)?.component}
        </div>
      </div>
    </div>
  );
} 