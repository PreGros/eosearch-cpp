cmake_minimum_required(VERSION 3.21)

project(geosearch-cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 

# Dependencies
find_package(nlohmann_json CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)

add_executable(geosearch
    src/main.cpp
)

target_include_directories(geosearch PRIVATE include)

target_link_libraries(geosearch PRIVATE
    nlohmann_json::nlohmann_json
    cpr::cpr
    httplib::httplib
)

if(MSVC)
    target_compile_options(geosearch PRIVATE /W4 /permissive- /utf-8)
    target_compile_definitions(geosearch PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(geosearch PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()
