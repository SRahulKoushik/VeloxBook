# tests/CMakeLists.txt

# Find all test source files matching test_*.cpp
file(GLOB TEST_SOURCES test_*.cpp)

# Create test executable
add_executable(orderbook_tests ${TEST_SOURCES})

# Link required libraries
target_link_libraries(orderbook_tests PRIVATE
    orderbook_shared          # Your core project library
    gtest                     # GoogleTest
    gtest_main                # GTest main()
    Drogon::Drogon           # Drogon framework
)

# Add include paths for test code
target_include_directories(orderbook_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external
)

# Enable automatic test discovery
include(GoogleTest)
gtest_discover_tests(orderbook_tests)


