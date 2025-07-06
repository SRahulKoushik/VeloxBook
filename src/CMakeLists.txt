# Build configuration for the VeloxBook backend
# This file tells CMake how to compile the C++ trading platform

# Find the libraries we need
find_package(Drogon REQUIRED)
find_package(OpenSSL REQUIRED)

# Core trading engine files
# These contain the matching engine, order book, and utility functions
set(ORDERBOOK_CORE
    matching_engine.cpp
    order_book.cpp
    order.hpp
    matching_engine.hpp
    order_book.hpp
    utils.hpp
    utils.cpp
)

# Bcrypt password hashing library
# I've included this locally so you don't need to install it separately
# It's used for securely hashing user passwords
set(BCRYPT_SOURCES
    ${CMAKE_SOURCE_DIR}/external/bcrypt/my_bcrypt.c
    ${CMAKE_SOURCE_DIR}/external/bcrypt/crypt_blowfish/crypt_blowfish.c
    ${CMAKE_SOURCE_DIR}/external/bcrypt/crypt_blowfish/crypt_gensalt.c
    ${CMAKE_SOURCE_DIR}/external/bcrypt/crypt_blowfish/wrapper.c
)

# Build bcrypt as a static library
add_library(bcrypt STATIC ${BCRYPT_SOURCES})

# Windows-specific compiler flags for bcrypt
target_compile_definitions(bcrypt PRIVATE
    _WIN32_WINNT=0x0601
    WIN32_LEAN_AND_MEAN
    __SKIP_GNU
    __USE_OW
)

# Include directories for bcrypt
target_include_directories(bcrypt PUBLIC
    ${CMAKE_SOURCE_DIR}/external/bcrypt
    ${CMAKE_SOURCE_DIR}/external/bcrypt/crypt_blowfish
)

# Main shared library containing the trading engine
# This includes the core engine plus the web API controllers
add_library(orderbook_shared STATIC
    ${ORDERBOOK_CORE}
    OrderBookController.cpp
    OrderBookController.h
    OrderBookWebSocket.cpp
    OrderBookWebSocket.h
    CorsFilter.h
)

# Link the shared library with Drogon and bcrypt
target_link_libraries(orderbook_shared PUBLIC Drogon::Drogon bcrypt)

# Include directories for the shared library
target_include_directories(orderbook_shared PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/external/jwt-cpp
)

# The main API server executable
# This is what you run to start the trading platform
add_executable(api_server
    api_server.cpp
    OrderBookController.cpp
    OrderBookController.h
    OrderBookWebSocket.cpp
    OrderBookWebSocket.h
)

# Link the server with all required libraries
target_link_libraries(api_server PRIVATE
    orderbook_shared
    Drogon::Drogon
    OpenSSL::SSL
    OpenSSL::Crypto
    bcrypt
)

# Include directories for the server
target_include_directories(api_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/external/bcrypt
    ${CMAKE_SOURCE_DIR}/external/bcrypt/crypt_blowfish
    ${CMAKE_SOURCE_DIR}/external/jwt-cpp
)



