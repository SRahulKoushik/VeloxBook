cmake_minimum_required(VERSION 3.10)
project(orderbook)

# Use C++17 for modern features like std::optional, std::variant, etc.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build settings that make development easier
# - Colored output in Ninja makes errors easier to spot
# - Debug symbols help with debugging
# - Export compile commands for IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

# Find the required libraries
# Drogon is the web framework, OpenSSL is for HTTPS/security
find_package(Drogon REQUIRED)
find_package(OpenSSL REQUIRED)

# Enable unit testing so you can run the test suite
enable_testing()

# Include GoogleTest from the external directory
# I've included it locally so you don't need to install it separately
add_subdirectory(external/googletest)

# Build the main source code and tests
add_subdirectory(src)
add_subdirectory(tests)


