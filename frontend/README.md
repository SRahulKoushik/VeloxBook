# Order Book Frontend

## Setup Instructions

1. **Install dependencies:**
   ```bash
   npm install
   ```

2. **Create a `.env` file in the `frontend/` directory:**
   ```env
   VITE_API_URL=http://localhost:18080
   VITE_WS_URL=ws://localhost:18080
   ```
   Adjust these URLs if your backend runs elsewhere.

3. **Start the development server:**
   ```bash
   npm run dev
   ```

4. **Required dependencies:**
   - react
   - react-dom
   - react-router-dom
   - axios
   - react-hot-toast
   - tailwindcss
   - postcss
   - autoprefixer
   - @vitejs/plugin-react

A React + Vite + Tailwind CSS frontend for the Order Book server/engine.

## Setup

1. Install dependencies:
   ```sh
   npm install
   ```
2. Copy `.env` and set your backend API URL if needed.

## Development

```sh
npm run dev
```

## Build

```sh
npm run build
```

## Preview Production Build

```sh
npm run preview
``` 