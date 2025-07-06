import React, { createContext, useContext, useState, useEffect } from 'react';
import { loginUser, registerUser } from './api';
import toast from 'react-hot-toast';
import { useNavigate } from 'react-router-dom';

// React context for authentication state
// This lets any component access the current user and auth functions
const AuthContext = createContext();

// Custom hook to use authentication context
// Throws an error if used outside of AuthProvider (safety check)
export function useAuth() {
  const context = useContext(AuthContext);
  if (!context) {
    throw new Error('useAuth must be used within an AuthProvider');
  }
  return context;
}

// Authentication provider component
// Manages user login state and provides auth functions to the entire app
// I've made it persistent so users stay logged in between page refreshes
export function AuthProvider({ children }) {
  const [user, setUser] = useState(null);
  const [loading, setLoading] = useState(true);
  const navigate = useNavigate ? useNavigate() : () => {};

  useEffect(() => {
    // Check if user is already logged in when the app starts
    // This restores the session from localStorage
    const token = localStorage.getItem('authToken');
    const userData = localStorage.getItem('userData');
    if (token && userData) {
      try {
        setUser(JSON.parse(userData));
      } catch (error) {
        console.error('Failed to parse user data:', error);
        // Clear corrupted data and redirect to login
        localStorage.removeItem('authToken');
        localStorage.removeItem('userData');
        setUser(null);
        toast.error('Session expired. Please log in again.');
        navigate('/login');
      }
    } else {
      setUser(null);
    }
    setLoading(false);
  }, []);

  // Login function - authenticates user and stores session
  const login = async (username, password) => {
    try {
      const response = await loginUser(username, password);
      const { token, user: userData } = response;
      
      // Store authentication data in localStorage
      // This keeps the user logged in even if they close the browser
      localStorage.setItem('authToken', token);
      localStorage.setItem('userData', JSON.stringify(userData));
      setUser(userData);
      
      toast.success('Login successful!');
      return true;
    } catch (error) {
      toast.error(error.message);
      return false;
    }
  };

  // Register function - creates new user account
  const register = async (username, password) => {
    try {
      await registerUser(username, password);
      toast.success('Registration successful! Please login.');
      return true;
    } catch (error) {
      throw error;
    }
  };

  // Logout function - clears session and redirects
  const logout = () => {
    localStorage.removeItem('authToken');
    localStorage.removeItem('userData');
    setUser(null);
    toast.success('Logged out successfully');
  };

  // Value object that gets passed to all child components
  const value = {
    user,        // Current user data (null if not logged in)
    login,       // Function to log in
    register,    // Function to register
    logout,      // Function to log out
    loading      // Whether we're still checking auth status
  };

  return (
    <AuthContext.Provider value={value}>
      {children}
    </AuthContext.Provider>
  );
}