import axios from 'axios';

// API base URL - gets it from environment variable or defaults to localhost
// This makes it easy to switch between development and production
export const API_URL = import.meta.env.VITE_API_URL || 'http://localhost:18080';

// Create axios instance with base configuration
// I've set it up to always use /api prefix and JSON content type
const api = axios.create({
  baseURL: API_URL + '/api',
  headers: {
    'Content-Type': 'application/json',
  },
});

// Automatically add JWT token to all requests if user is logged in
// This way you don't have to manually add the token to every API call
api.interceptors.request.use((config) => {
  const token = localStorage.getItem('authToken');
  if (token) {
    config.headers.Authorization = `Bearer ${token}`;
  }
  return config;
});

// Place a new order on the exchange
// This is the main trading function - sends order to the matching engine
export async function placeOrder(order) {
  try {
    const response = await api.post('/order', order);
    return response.data;
  } catch (error) {
    throw new Error(error.response?.data?.error || 'Failed to place order');
  }
}

// Get the current order book for a symbol
// Returns bids and asks - what you see in the order book display
export async function getOrderBook(symbol) {
  try {
    const response = await api.get(`/orderbook/${encodeURIComponent(symbol)}`);
    return response.data;
  } catch (error) {
    throw new Error(error.response?.data?.error || 'Failed to fetch order book');
  }
}

// Get a user's trade history
// Shows all the trades they've made - useful for tracking performance
export async function getTradeHistory(userId) {
  try {
    const response = await api.get(`/trades/${encodeURIComponent(userId)}`);
    return response.data;
  } catch (error) {
    throw new Error(error.response?.data?.error || 'Failed to fetch trade history');
  }
}

// Get all open orders for a user
// Shows orders that haven't been filled or cancelled yet
export async function getMyOrders(userId) {
  try {
    const response = await api.get(`/orders/${encodeURIComponent(userId)}`);
    return response.data;
  } catch (error) {
    throw new Error(error.response?.data?.error || 'Failed to fetch orders');
  }
}

// Cancel an existing order
// Removes the order from the book if it hasn't been filled
export async function cancelOrder(orderId) {
  try {
    const response = await api.delete(`/cancel/${encodeURIComponent(orderId)}`);
    return response.data;
  } catch (error) {
    throw new Error(error.response?.data?.error || 'Failed to cancel order');
  }
}

// Modify an existing order's price and quantity
// This is like cancelling and placing a new order, but atomic
export async function modifyOrder(orderId, price, quantity) {
  try {
    const response = await api.post('/modify', {
      order_id: orderId,
      price: price,
      quantity: quantity
    });
    return response.data;
  } catch (error) {
    throw new Error(error.response?.data?.error || 'Failed to modify order');
  }
}

// Get details of a specific order by ID
// Useful for checking order status or getting order information
export async function getOrderById(orderId) {
  try {
    const response = await api.get(`/order/${encodeURIComponent(orderId)}`);
    return response.data;
  } catch (error) {
    throw new Error(error.response?.data?.error || 'Failed to fetch order');
  }
}

// Check if the server is running and healthy
// Good for monitoring and debugging connection issues
export async function healthCheck() {
  try {
    const response = await api.get('/health');
    return response.data;
  } catch (error) {
    throw new Error('Server health check failed');
  }
}

// Register a new user account
// Creates a new user with username and password
export async function registerUser(username, password) {
  try {
    const response = await api.post('/register', { username, password });
    return response.data;
  } catch (error) {
    throw new Error(error.response?.data?.error || 'Failed to register user');
  }
}

// Log in an existing user
// Returns a JWT token that gets stored for future API calls
export async function loginUser(username, password) {
  try {
    const response = await api.post('/login', { username, password });
    return response.data;
  } catch (error) {
    throw new Error(error.response?.data?.error || 'Failed to login');
  }
}