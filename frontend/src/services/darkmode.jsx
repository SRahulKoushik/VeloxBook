import React, { createContext, useContext, useEffect, useState } from 'react';

// Dark mode context for theme switching
// This lets any component toggle between light and dark themes
export const DarkModeContext = createContext();

// Dark mode provider component
// Manages the dark/light theme state and applies it to the entire app
// I've made it remember the user's preference and respect their system setting
export function DarkModeProvider({ children }) {
  // Initialize dark mode state
  // First checks localStorage, then falls back to system preference
  const [dark, setDark] = useState(() => {
    const stored = localStorage.getItem('darkMode');
    if (stored !== null) return stored === 'true';
    return window.matchMedia('(prefers-color-scheme: dark)').matches;
  });

  // Apply dark mode class to HTML element and save preference
  // This is what actually changes the theme across the entire app
  useEffect(() => {
    if (dark) document.documentElement.classList.add('dark');
    else document.documentElement.classList.remove('dark');
    localStorage.setItem('darkMode', dark);
  }, [dark]);

  // Toggle function to switch between themes
  const toggleDark = () => setDark(d => !d);

  return (
    <DarkModeContext.Provider value={{ dark, toggleDark }}>
      {children}
    </DarkModeContext.Provider>
  );
} 