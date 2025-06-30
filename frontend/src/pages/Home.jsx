import React from 'react';
import { Link } from 'react-router-dom';
import { useAuth } from '../services/auth.jsx';

export default function Home() {
  const { user } = useAuth();

  return (
    <div className="min-h-screen bg-gradient-to-br from-indigo-100 to-blue-100 flex items-center justify-center px-4 sm:px-6 lg:px-8">
      <div className="max-w-4xl mx-auto text-center">
        <h1 className="text-3xl sm:text-4xl lg:text-5xl font-bold text-gray-900 mb-4 sm:mb-6">
          Order Book Trading Platform
        </h1>
        <p className="text-lg sm:text-xl text-gray-600 mb-6 sm:mb-8 px-4">
          Professional-grade order book with real-time trading capabilities
        </p>
        
        <div className="space-y-4 sm:space-y-0 sm:space-x-4 sm:flex sm:justify-center">
          {user ? (
            <Link
              to="/dashboard"
              className="inline-block w-full sm:w-auto bg-indigo-600 text-white px-6 sm:px-8 py-3 rounded-lg text-base sm:text-lg font-semibold hover:bg-indigo-700 transition-colors"
            >
              Go to Dashboard
            </Link>
          ) : (
            <>
              <Link
                to="/login"
                className="inline-block w-full sm:w-auto bg-indigo-600 text-white px-6 sm:px-8 py-3 rounded-lg text-base sm:text-lg font-semibold hover:bg-indigo-700 transition-colors"
              >
                Sign In
              </Link>
              <Link
                to="/register"
                className="inline-block w-full sm:w-auto bg-white text-indigo-600 px-6 sm:px-8 py-3 rounded-lg text-base sm:text-lg font-semibold border-2 border-indigo-600 hover:bg-indigo-50 transition-colors"
              >
                Create Account
              </Link>
            </>
          )}
        </div>

        <div className="mt-12 sm:mt-16 grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-6 sm:gap-8">
          <div className="bg-white p-4 sm:p-6 rounded-lg shadow-md">
            <h3 className="text-lg sm:text-xl font-semibold mb-2">Real-time Order Book</h3>
            <p className="text-sm sm:text-base text-gray-600">Live order book updates with WebSocket connectivity</p>
          </div>
          <div className="bg-white p-4 sm:p-6 rounded-lg shadow-md">
            <h3 className="text-lg sm:text-xl font-semibold mb-2">Secure Trading</h3>
            <p className="text-sm sm:text-base text-gray-600">JWT authentication and secure order placement</p>
          </div>
          <div className="bg-white p-4 sm:p-6 rounded-lg shadow-md sm:col-span-2 lg:col-span-1">
            <h3 className="text-lg sm:text-xl font-semibold mb-2">Trade History</h3>
            <p className="text-sm sm:text-base text-gray-600">Complete trade history and order management</p>
          </div>
        </div>
      </div>
    </div>
  );
} 