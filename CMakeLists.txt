cmake_minimum_required(VERSION 3.25)
project(budouxpp VERSION 0.1.0 LANGUAGES CXX)

option(ENABLE_TESTS "Enable tests" ON)
option(ENABLE_EXAMPLE "Build example executable" ON)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++26" COMPILER_SUPPORTS_CXX26)
CHECK_CXX_COMPILER_FLAG("-std=c++23" COMPILER_SUPPORTS_CXX23)
CHECK_CXX_COMPILER_FLAG("-std=c++20" COMPILER_SUPPORTS_CXX20)
if(COMPILER_SUPPORTS_CXX26)
    set(STD_CPP cxx_std_26)
elseif(COMPILER_SUPPORTS_CXX23)
    set(STD_CPP cxx_std_23)
elseif(COMPILER_SUPPORTS_CXX20)
    set(STD_CPP cxx_std_20)
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++20 support. Please use a different C++ compiler.")
endif()

include(GNUInstallDirs)

find_package(glaze REQUIRED CONFIG)

add_library(budouxpp INTERFACE)
target_include_directories(budouxpp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(budouxpp INTERFACE glaze::glaze)
target_compile_features(budouxpp INTERFACE cxx_std_20)

if(ENABLE_EXAMPLE)
    find_package(quill REQUIRED CONFIG)
    add_executable(example main.cpp)
    target_link_libraries(example PRIVATE budouxpp quill::quill)
    target_compile_features(example PRIVATE ${STD_CPP})
endif()

if(ENABLE_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
