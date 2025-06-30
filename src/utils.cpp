#include "utils.hpp"
#include <stdexcept>
#include <cstring>
#include <cstdlib>
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace orderbook {

std::string bcrypt_hash_password(const std::string& password, int workfactor) {
    char salt[BCRYPT_HASHSIZE] = {0};
    char hash[BCRYPT_HASHSIZE] = {0};
    if (bcrypt_gensalt(workfactor, salt) != 0) {
        throw std::runtime_error("Failed to generate bcrypt salt");
    }
    if (bcrypt_hashpw(password.c_str(), salt, hash) != 0) {
        throw std::runtime_error("Failed to hash password with bcrypt");
    }
    return std::string(hash);
}

bool bcrypt_check_password(const std::string& password, const std::string& hash) {
    int ret = bcrypt_checkpw(password.c_str(), hash.c_str());
    if (ret == -1) {
        throw std::runtime_error("Error checking bcrypt password");
    }
    return ret == 0;
}

// Generate a secure random JWT secret for production
std::string generate_secure_jwt_secret() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    for (int i = 0; i < 64; ++i) { // 64 bytes = 512 bits
        ss << std::setw(2) << dis(gen);
    }
    
    return ss.str();
}

// Get JWT secret - use environment variable in production
std::string get_jwt_secret() {
    const char* env_secret = std::getenv("JWT_SECRET");
    if (env_secret) {
        return std::string(env_secret);
    }
    
    // Fallback to generated secret (not recommended for production)
    static std::string generated_secret = generate_secure_jwt_secret();
    return generated_secret;
}

} // namespace orderbook 