import axios from 'axios';

export const API_URL = import.meta.env.VITE_API_URL || 'http://localhost:18080';

// Always use /api prefix for backend endpoints
const api = axios.create({
  baseURL: API_URL + '/api',
  headers: {
    'Content-Type': 'application/json',
  },
});

// Add auth token to requests if available
api.interceptors.request.use((config) => {
  const token = localStorage.getItem('authToken');
  if (token) {
    config.headers.Authorization = `Bearer ${token}`;
  }
  return config;
});

// Place a new order
export async function placeOrder(order) {
  try {
    const response = await api.post('/order', order);
    return response.data;
  } catch (error) {
    throw new Error(error.response?.data?.error || 'Failed to place order');
  }
}

// Get order book (bids/asks)
export async function getOrderBook(symbol) {
  try {
    const response = await api.get(`/orderbook/${encodeURIComponent(symbol)}`);
    return response.data;
  } catch (error) {
    throw new Error(error.response?.data?.error || 'Failed to fetch order book');
  }
}

// Get trade history
export async function getTradeHistory(userId) {
  try {
    const response = await api.get(`/trades/${encodeURIComponent(userId)}`);
    return response.data;
  } catch (error) {
    throw new Error(error.response?.data?.error || 'Failed to fetch trade history');
  }
}

// Get all open orders for a user
export async function getMyOrders(userId) {
  try {
    const response = await api.get(`/orders/${encodeURIComponent(userId)}`);
    return response.data;
  } catch (error) {
    throw new Error(error.response?.data?.error || 'Failed to fetch orders');
  }
}

// Cancel an order by ID
export async function cancelOrder(orderId) {
  try {
    const response = await api.delete(`/cancel/${encodeURIComponent(orderId)}`);
    return response.data;
  } catch (error) {
    throw new Error(error.response?.data?.error || 'Failed to cancel order');
  }
}

// Modify an order
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

// Get specific order by ID
export async function getOrderById(orderId) {
  try {
    const response = await api.get(`/order/${encodeURIComponent(orderId)}`);
    return response.data;
  } catch (error) {
    throw new Error(error.response?.data?.error || 'Failed to fetch order');
  }
}

// Health check
export async function healthCheck() {
  try {
    const response = await api.get('/health');
    return response.data;
  } catch (error) {
    throw new Error('Server health check failed');
  }
}

// User registration
export async function registerUser(username, password) {
  try {
    const response = await api.post('/register', { username, password });
    return response.data;
  } catch (error) {
    throw new Error(error.response?.data?.error || 'Failed to register user');
  }
}

// User login
export async function loginUser(username, password) {
  try {
    const response = await api.post('/login', { username, password });
    return response.data;
  } catch (error) {
    throw new Error(error.response?.data?.error || 'Failed to login');
  }
}