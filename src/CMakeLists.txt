# src/CMakeLists.txt

# Dependencies
find_package(Drogon REQUIRED)
find_package(OpenSSL REQUIRED)

# Core library files
set(ORDERBOOK_CORE
    matching_engine.cpp
    order_book.cpp
    order.hpp
    matching_engine.hpp
    order_book.hpp
    utils.hpp
    utils.cpp
)

# Bcrypt sources
set(BCRYPT_SOURCES
    ${CMAKE_SOURCE_DIR}/external/bcrypt/my_bcrypt.c
    ${CMAKE_SOURCE_DIR}/external/bcrypt/crypt_blowfish/crypt_blowfish.c
    ${CMAKE_SOURCE_DIR}/external/bcrypt/crypt_blowfish/crypt_gensalt.c
    ${CMAKE_SOURCE_DIR}/external/bcrypt/crypt_blowfish/wrapper.c
)

add_library(bcrypt STATIC ${BCRYPT_SOURCES})

target_compile_definitions(bcrypt PRIVATE
    _WIN32_WINNT=0x0601
    WIN32_LEAN_AND_MEAN
    __SKIP_GNU
    __USE_OW
)

target_include_directories(bcrypt PUBLIC
    ${CMAKE_SOURCE_DIR}/external/bcrypt
    ${CMAKE_SOURCE_DIR}/external/bcrypt/crypt_blowfish
)

add_library(orderbook_shared STATIC
    ${ORDERBOOK_CORE}
    OrderBookController.cpp
    OrderBookController.h
    OrderBookWebSocket.cpp
    OrderBookWebSocket.h
    CorsFilter.h
)

target_link_libraries(orderbook_shared PUBLIC Drogon::Drogon bcrypt)

target_include_directories(orderbook_shared PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/external/jwt-cpp
)

# API server executable
add_executable(api_server
    api_server.cpp
    OrderBookController.cpp
    OrderBookController.h
    OrderBookWebSocket.cpp
    OrderBookWebSocket.h
)

target_link_libraries(api_server PRIVATE
    orderbook_shared
    Drogon::Drogon
    OpenSSL::SSL
    OpenSSL::Crypto
    bcrypt
)

target_include_directories(api_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/external/bcrypt
    ${CMAKE_SOURCE_DIR}/external/bcrypt/crypt_blowfish
    ${CMAKE_SOURCE_DIR}/external/jwt-cpp
)



