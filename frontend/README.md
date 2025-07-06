# VeloxBook Frontend

This is the React frontend for the VeloxBook trading platform. I built it with Vite for fast development and Tailwind CSS because I'm not a designer but still want it to look decent.

## Quick Start

1. **Install the dependencies:**
   ```bash
   npm install
   ```

2. **Set up your environment:**
   Create a `.env` file in the `frontend/` directory:
   ```env
   VITE_API_URL=http://localhost:18080
   VITE_WS_URL=ws://localhost:18080
   ```
   Change these URLs if your backend is running on a different machine or port.

3. **Start the development server:**
   ```bash
   npm run dev
   ```

4. **Open your browser:**
   The app should be running at `http://localhost:5173`

## What's Included

- **React 18** - Modern React with hooks and functional components
- **Vite** - Super fast build tool and dev server
- **Tailwind CSS** - Utility-first CSS framework (makes styling actually bearable)
- **React Router** - Client-side routing
- **Axios** - HTTP client for API calls
- **React Hot Toast** - Nice toast notifications
- **WebSocket** - Real-time updates for the order book

## Development

```bash
npm run dev          # Start development server
npm run build        # Build for production
npm run preview      # Preview the production build locally
```

## Project Structure

```
src/
├── components/      # Reusable UI components
│   ├── OrderBook.jsx    # Live order book display
│   ├── OrderForm.jsx    # Order placement form
│   ├── TradeHistory.jsx # Trade history table
│   ├── MyOrders.jsx     # User's orders management
│   ├── Header.jsx       # Navigation and user controls
│   └── TickerTape.jsx   # Market summary (optional)
├── pages/          # Page components
│   ├── Dashboard.jsx    # Main trading interface
│   ├── Home.jsx         # Landing page
│   ├── Login.jsx        # User login
│   └── Register.jsx     # User registration
├── services/       # API and utility services
│   ├── api.js          # REST API client
│   ├── auth.jsx        # Authentication logic
│   ├── darkmode.jsx    # Dark mode toggle
│   └── useOrderBookSocket.jsx  # WebSocket hook
└── index.jsx       # App entry point
```

## Key Features

- **Real-time Order Book:** Live updates via WebSocket, no page refreshes needed
- **Order Placement:** Clean forms for market and limit orders with validation
- **Trade History:** Sortable table showing your executed trades
- **User Management:** Login/register with JWT authentication
- **Responsive Design:** Works on desktop, tablet, and mobile
- **Dark Mode:** Toggle between light and dark themes

## Configuration

The main configuration is in the `.env` file:

- `VITE_API_URL` - Backend REST API URL
- `VITE_WS_URL` - Backend WebSocket URL

You can also customize:
- `tailwind.config.js` - Tailwind CSS configuration
- `vite.config.js` - Vite build configuration
- `postcss.config.js` - PostCSS plugins

## Building for Production

```bash
npm run build
```

This creates a `dist/` folder with optimized files ready to deploy. You can serve these files with any static file server.

## Troubleshooting

### "Module not found" errors
- Run `npm install` to install dependencies
- Check that all imports are correct

### WebSocket connection issues
- Make sure your backend is running
- Check the `VITE_WS_URL` in your `.env` file
- Verify your firewall isn't blocking WebSocket connections

### API calls failing
- Check the `VITE_API_URL` in your `.env` file
- Make sure your backend is running and accessible
- Check the browser console for CORS errors

## Notes

- The frontend is designed to work with the VeloxBook backend
- WebSocket connections are automatically reconnected if they drop
- All API calls include proper error handling and user feedback
- The UI is responsive and works on mobile devices 