import React from 'react';
import { BrowserRouter as Router, Routes, Route, Navigate } from 'react-router-dom';
import { Toaster } from 'react-hot-toast';
import Header from './components/Header';
import TickerTape from './components/TickerTape';
import Login from './pages/Login';
import Register from './pages/Register';
import Dashboard from './pages/Dashboard';
import Home from './pages/Home';
import { AuthProvider, useAuth } from './services/auth.jsx';
import { DarkModeProvider } from './services/darkmode.jsx';

// Protected route component - checks if user is logged in
// If not logged in, redirects to login page
// Shows a loading spinner while checking auth status
function PrivateRoute({ children }) {
  const { user, loading } = useAuth();
  
  if (loading) {
    return (
      <div className="min-h-screen flex justify-center items-center">
        <div className="text-center">
          <div className="animate-spin rounded-full h-12 w-12 border-b-2 border-indigo-600 mx-auto"></div>
          <p className="mt-4 text-gray-600">Loading...</p>
        </div>
      </div>
    );
  }
  
  return user ? children : <Navigate to="/login" />;
}

// Main app component - this is where everything comes together
// I've structured it with providers at the top level so all components
// can access auth state and dark mode settings
export default function App() {
  return (
    <DarkModeProvider>
      <Router>
        <AuthProvider>
          <div className="min-h-screen bg-gray-50 dark:bg-gray-950">
            {/* Toast notifications for user feedback */}
            <Toaster 
              position="top-right"
              toastOptions={{
                duration: 4000,
                style: {
                  background: '#363636',
                  color: '#fff',
                },
              }}
            />
            <Header />
            <main className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-4 sm:py-6">
              <TickerTape />
              {/* App routing - different pages for different features */}
              <Routes>
                <Route path="/" element={<Home />} />
                <Route path="/login" element={<Login />} />
                <Route path="/register" element={<Register />} />
                <Route path="/dashboard" element={<PrivateRoute><Dashboard /></PrivateRoute>} />
              </Routes>
            </main>
          </div>
        </AuthProvider>
      </Router>
    </DarkModeProvider>
  );
} 