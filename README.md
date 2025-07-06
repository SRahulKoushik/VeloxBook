# VeloxBook

Hey there! I built this trading platform because I wanted something that could handle real-time order book data without the usual latency issues. It's a full-stack system with a C++ backend that's fast as lightning and a React frontend that actually looks good.

I've put a lot of work into making sure the WebSocket connections are rock solid and the order matching engine can handle serious volume.

---

## What's Inside

- [Features](#features)
- [How It Works](#how-it-works)
- [Backend Details](#backend-details)
- [Frontend Details](#frontend-details)
- [Getting Started](#getting-started)
- [API Reference](#api-reference)
- [Dependencies](#dependencies)
- [Important Notes](#important-notes)
- [License](#license)
- [Support](#support)

---

## Features

Here's what makes this platform special:

- **Real-time Order Book:** WebSocket updates that actually work - no more refreshing pages to see the latest prices
- **Secure Trading:** JWT auth, rate limiting, and proper order validation so you don't accidentally blow up your account
- **All Order Types:** Market, limit, stop, stop-limit orders with proper TIF (Time-in-Force) handling
- **Trade History:** Everything gets logged and you can query your own trades
- **Live Metrics:** Spread, order/trade ratios, cancellation rates - all the stuff you need to know
- **Responsive UI:** Works on desktop, tablet, phone - wherever you want to trade
- **Extensible:** Easy to add new features or markets when you need them

---

## How It Works

```
React Frontend ←→ C++ Backend ←→ PostgreSQL
     ↓              ↓              ↓
  WebSocket     Matching Engine   Data Storage
  REST API      Order Processing  Trade History
```

The frontend talks to the backend via REST API and WebSocket. The backend handles all the heavy lifting with a custom matching engine, and everything gets stored in PostgreSQL so you don't lose your data.

---

## Backend Details

I built the backend in C++ using Drogon because I needed something that could handle thousands of orders per second without breaking a sweat. Here's what's under the hood:

- **Matching Engine:** Thread-safe order book that can handle multiple symbols and order types
- **API Server:** REST endpoints for everything you need - orders, trades, user management
- **WebSocket:** Real-time updates that actually work (no polling nonsense)
- **Database:** PostgreSQL for persistence - your orders and trades are safe
- **Security:** JWT tokens, bcrypt password hashing, proper CORS setup

### API Endpoints

Here are the main endpoints you'll use:

- `POST /order` — Place orders (market, limit, stop, stop-limit)
- `DELETE /cancel/{order_id}` — Cancel an order
- `POST /modify` — Modify existing orders
- `GET /orders/{user_id}` — Get your order history
- `GET /orderbook/{symbol}` — Get current order book
- `GET /order/{order_id}` — Get specific order details
- `GET /trades/{user_id}` — Get your trade history
- `POST /register` — Create account
- `POST /login` — Get JWT token
- `GET /health` — Check if server is alive
- `GET /metrics` — Get performance metrics
- `GET /async_demo` — See the async/concurrency features in action

### WebSocket

- `/ws/orderbook` — Subscribe to real-time order book and trade updates

---

## Frontend Details

The frontend is built with React and Vite - fast development and fast runtime. I used Tailwind CSS because I'm not a designer and it makes everything look decent.

- **Live Order Book:** Real-time updates, responsive tables, all the good stuff
- **Order Placement:** Clean forms with proper validation
- **Trade History:** Sortable, filterable, shows exactly what you need
- **Authentication:** JWT-based login/register with protected routes
- **UI/UX:** Responsive design, dark mode support
- **Notifications:** Toast messages for order status and errors

### Components

- `OrderBook` — Shows live bids/asks with real-time updates
- `OrderForm` — Place buy/sell orders with validation
- `TradeHistory` — Your executed trades
- `MyOrders` — Manage open and historical orders
- `TickerTape` — Market summary (optional)
- `Header` — Navigation and user controls

---

## Getting Started

### What You Need

- C++17 compiler, Ninja, CMake, Drogon, PostgreSQL
- Node.js (v16+), npm

### Backend Setup

```sh
# From project root
mkdir build
cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="path-to-drogon-installation"
ninja
cd src
# Start the server (make sure PostgreSQL is running)
.\api_server.exe
```

### Frontend Setup

```sh
cd frontend
npm install
cp .env.example .env # Update API/WS URLs if needed
npm run dev
```

---

## Async & Concurrency Features

I've implemented proper async/concurrent programming in the backend because trading platforms need to handle multiple requests simultaneously. Here's what I used:

- **std::async, std::future:** Background tasks that don't block the main thread
- **std::thread:** Custom threads for heavy work
- **std::mutex, std::lock_guard:** Thread safety for shared data
- **std::condition_variable:** Thread coordination
- **Drogon async handlers:** Non-blocking HTTP endpoints
- **Async database access:** SQL queries that don't freeze the server
- **Thread pool:** Drogon handles concurrent requests efficiently

Check out the `/async_demo` endpoint to see this stuff in action.

---

## API Reference

The full API details are in the backend source files (`src/OrderBookController.h/cpp`). That's where you'll find the exact request/response formats.

---

## Dependencies

I've included these third-party libraries:

- **bcrypt** — Password hashing (see [external/bcrypt/README](external/bcrypt/README))
- **jwt-cpp** — JWT handling (see [external/jwt-cpp/](external/jwt-cpp/))
- **picojson** — JSON parsing (see [external/picojson/](external/picojson/))
- **googletest** — C++ unit testing (see [external/googletest/README.md](external/googletest/README.md))

Each library has its own license and documentation. If you have issues with bcrypt or googletest, you might need to clone them manually.

---

## Important Notes

- Create .env files with your environment variables in the frontend and root directories
- Put your SSL certificates in the certs folder
- The WebSocket latency test shows some pretty impressive results - check out `latency_test/README.md`

---

## License

This project uses the MIT License. The third-party libraries keep their original licenses (see [LICENSE](LICENSE)).

---

## Support

- Frontend details: [frontend/README.md](frontend/README.md)
- Backend config: `src/config.json`
- Tests: `tests/`
- WebSocket performance test: `latency_test/README.md`
- Questions or contributions? Open an issue or pull request.

---

## Backend Asynchronous & Concurrency Features

The backend demonstrates modern C++ and Drogon asynchronous/concurrent programming:

- **std::async, std::future:** Launch background tasks and retrieve results asynchronously.
- **std::thread:** Run custom background threads for offloading work.
- **std::mutex, std::lock_guard:** Ensure thread safety for shared data.
- **std::condition_variable:** Signal and coordinate between threads.
- **Drogon async handlers:** Use callback-based async HTTP endpoints for non-blocking request processing.
- **Asynchronous database access:** Drogon ORM supports fully async SQL queries.
- **Thread pool:** Drogon runs handlers in a thread pool for high concurrency.

See the `/async_demo` endpoint for a live demonstration of these features.


