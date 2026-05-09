cmake_minimum_required(VERSION 3.25)
project(budouxpp_find_package_smoke LANGUAGES CXX)

find_package(budouxpp CONFIG REQUIRED)

add_executable(smoke main.cpp)
target_link_libraries(smoke PRIVATE budouxpp::budouxpp)
