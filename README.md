# VeloxBook

A full-stack, professional-grade order book trading platform featuring a high-performance C++ backend and a modern React frontend. Designed for real-time trading, robust order management, and extensibility.

---

## Table of Contents

- [Features](#features)
- [Architecture Overview](#architecture-overview)
- [Backend (C++/Drogon)](#backend-cdrogon)
- [Frontend (React/Vite)](#frontend-reactvite)
- [Setup & Installation](#setup--installation)
- [API Overview](#api-overview)
- [Third-Party Dependencies](#third-party-dependencies)
- [Caution](#caution)
- [License](#license)
- [More Information](#more-information)

---

## Features

- **Real-time Order Book:** Live updates via WebSocket for instant market depth and trade visibility.
- **Secure Trading:** JWT-based authentication, rate limiting, and secure order placement.
- **Comprehensive Order Types:** Supports market, limit, stop, and stop-limit orders, with TIF (Time-in-Force) options (GTC, IOC, FOK).
- **Trade History:** Persistent trade and order history, with user-specific queries.
- **Metrics & Analytics:** Real-time metrics (spread, order/trade ratio, cancellation rate) and health endpoints.
- **Responsive UI:** Modern, mobile-friendly interface with live order book, trade history, and order management.
- **Extensible:** Modular backend and frontend, easy to add new features or markets.

---

## Architecture Overview

  A[React Frontend] --REST/WebSocket--> B[Drogon C++ API Server]
  B --DB access--> C[(PostgreSQL)]
  B --Password Hashing--> D[bcrypt]
  B --JWT Auth--> E[jwt-cpp]
  B --JSON--> F[picojson]

- **Frontend:** React (Vite, Tailwind CSS) SPA, communicates with backend via REST and WebSocket.
- **Backend:** Drogon C++ server, custom matching engine, PostgreSQL for persistence, JWT for auth, bcrypt for password security.

---

## Backend (C++/Drogon)

- **Matching Engine:** High-performance, thread-safe order book supporting multiple symbols, price levels, and order types.
- **API Server:** Built with Drogon, exposes REST endpoints for order management, user auth, and metrics.
- **WebSocket:** Real-time order book and trade updates.
- **Persistence:** PostgreSQL for orders, actions, and trade history.
- **Security:** JWT authentication, bcrypt password hashing, CORS, and rate limiting.

### Key Backend Endpoints

- `POST /order` — Place a new order (market, limit, stop, stop-limit)
- `DELETE /cancel/{order_id}` — Cancel an order
- `POST /modify` — Modify an existing order
- `GET /orders/{user_id}` — List user's orders (with filters)
- `GET /orderbook/{symbol}` — Get current order book for a symbol
- `GET /order/{order_id}` — Get order details
- `GET /trades/{user_id}` — Get user's trade history
- `POST /register` — Register a new user
- `POST /login` — Authenticate and receive JWT
- `GET /health` — Health check
- `GET /metrics` — Real-time metrics

### WebSocket

- `/ws/orderbook` — Subscribe for real-time order book and trade updates

---

## Frontend (React/Vite)

- **Live Order Book:** Real-time updates with WebSocket, responsive tables for bids/asks.
- **Order Placement:** Secure, user-friendly form for market/limit orders.
- **Trade History:** User-specific trade history, sortable and filterable.
- **Authentication:** JWT-based login/register, protected routes.
- **UI/UX:** Built with Tailwind CSS, responsive, dark mode ready.
- **Notifications:** Toasts for order status, errors, and system messages.

### Main Components

- `OrderBook` — Displays live bids/asks, updates in real time
- `OrderForm` — Place buy/sell orders with validation
- `TradeHistory` — Shows user's executed trades
- `MyOrders` — Manage open and historical orders
- `TickerTape` — (Optional) Market ticker/summary
- `Header` — Navigation and user controls

---

## Setup & Installation

### Prerequisites

- C++17 compiler, Ninja, CMake, Drogon, PostgreSQL
- Node.js (v16+), npm

### Backend

```sh
# From project root
mkdir build
cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="path-to-drogon-installation"
ninja
cd src
# Run the server (ensure PostgreSQL is running and configured)
.\api_server.exe
```

### Frontend

```sh
cd frontend
npm install
cp .env.example .env # Edit API/WS URLs if needed
npm run dev
```

---

## API Overview

See the backend source (`src/OrderBookController.h/cpp`) for full endpoint details and request/response formats.

---

## Third-Party Dependencies

- **bcrypt** — Password hashing ([external/bcrypt/README](external/bcrypt/README))
- **jwt-cpp** — JWT handling ([external/jwt-cpp/](external/jwt-cpp/))
- **picojson** — JSON parsing ([external/picojson/](external/picojson/))
- **googletest** — C++ unit testing ([external/googletest/README.md](external/googletest/README.md))

See each library's directory for license and documentation details.
Clone bcrypt and googletest libraries manually if you have any issues.

---

## Caution

Create .env files with the required environment variables in the frontend and root directories.
Add your SSL Certificates to the certs folder.

---

## License

This project is licensed under the MIT License.  
Third-party libraries retain their original licenses (see [LICENSE](LICENSE)).

---

## More Information

- Frontend details: [frontend/README.md](frontend/README.md)
- Backend configuration: `src/config.json`
- Test instructions: `tests/`
- For questions or contributions, open an issue or pull request.
