# Subscription Tracker - cross-platform CMake build.
#
#   cmake -S . -B build
#   cmake --build build --config Release
#
# Set SUBSCRIPTION_TRACKER_WITH_SQLITE=ON to enable the optional
# SQL backend (+3 points). It is OFF by default so the project
# builds with just a C++17 compiler.

cmake_minimum_required(VERSION 3.16)
project(SubscriptionTracker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(SUBSCRIPTION_TRACKER_WITH_SQLITE
       "Enable SQLite repository (+3 bonus points)" OFF)

add_executable(SubscriptionTracker
    src/main.cpp
    src/Subscription.cpp
    src/SubscriptionManager.cpp
    src/FileIO.cpp
)

target_include_directories(SubscriptionTracker PRIVATE src)

if(SUBSCRIPTION_TRACKER_WITH_SQLITE)
    find_package(SQLite3 REQUIRED)
    target_sources(SubscriptionTracker PRIVATE src/SqlRepository.cpp)
    target_compile_definitions(SubscriptionTracker PRIVATE
        SUBSCRIPTION_TRACKER_WITH_SQLITE=1)
    target_link_libraries(SubscriptionTracker PRIVATE SQLite::SQLite3)
endif()

# Copy the sample data file next to the binary on every build so
# the "load on startup" requirement works out of the box.
add_custom_command(TARGET SubscriptionTracker POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:SubscriptionTracker>/data)
