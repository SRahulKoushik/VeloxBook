cmake_minimum_required(VERSION 3.10)
project(orderbook)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ensure colored Ninja output and Debug symbols
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

# Find Drogon + OpenSSL
find_package(Drogon REQUIRED)
find_package(OpenSSL REQUIRED)

# Enable unit testing
enable_testing()

# GoogleTest (from local clone)
add_subdirectory(external/googletest)

# Subprojects
add_subdirectory(src)
add_subdirectory(tests)


