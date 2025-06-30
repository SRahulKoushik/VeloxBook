#ifndef ORDERBOOK_UTILS_HPP
#define ORDERBOOK_UTILS_HPP

#include <cstdint>
#include <string>
#include <chrono>

extern "C" {
#include "my_bcrypt.h"
}

namespace orderbook {

// Common type aliases
using OrderId = std::string;
using UserId = std::string;
using Price = uint64_t;
using Quantity = uint64_t;

// Constants
constexpr Quantity MAX_ORDER_QUANTITY = 1'000'000;
constexpr Price MAX_ORDER_PRICE = 1'000'000;

// Time utilities
inline uint64_t now_nanoseconds() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

// Bcrypt password hashing utilities
std::string bcrypt_hash_password(const std::string& password, int workfactor = 12);
bool bcrypt_check_password(const std::string& password, const std::string& hash);

// JWT secret utility
std::string get_jwt_secret();

} // namespace orderbook

#endif // ORDERBOOK_UTILS_HPP
