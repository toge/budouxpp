cmake_minimum_required(VERSION 3.25)

option(ENABLE_TESTS "Enable tests" ON)
option(ENABLE_EXAMPLE "Build example executable" ON)
option(ENABLE_BENCHMARK "Build benchmark executable" OFF)

set(_budouxpp_vcpkg_manifest_features "")
if(ENABLE_TESTS)
    list(APPEND _budouxpp_vcpkg_manifest_features "tests")
endif()
if(ENABLE_EXAMPLE)
    list(APPEND _budouxpp_vcpkg_manifest_features "example")
endif()
if(ENABLE_BENCHMARK)
    list(APPEND _budouxpp_vcpkg_manifest_features "benchmark")
endif()
set(VCPKG_MANIFEST_NO_DEFAULT_FEATURES ON CACHE BOOL
    "Resolve vcpkg dependencies only from CMake-selected manifest features" FORCE)
set(VCPKG_MANIFEST_FEATURES "${_budouxpp_vcpkg_manifest_features}" CACHE STRING
    "vcpkg manifest features required by the current CMake options" FORCE)
unset(_budouxpp_vcpkg_manifest_features)

project(budouxpp VERSION 0.1.0 LANGUAGES CXX)

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
include(CMakePackageConfigHelpers)

find_package(glaze REQUIRED CONFIG)
find_package(unordered_dense REQUIRED CONFIG)

add_library(budouxpp INTERFACE)
target_include_directories(budouxpp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(budouxpp INTERFACE glaze::glaze unordered_dense::unordered_dense)
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

if(ENABLE_BENCHMARK)
    add_subdirectory(benchmark)
endif()

install(TARGETS budouxpp EXPORT budouxppTargets)
install(DIRECTORY budoux/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/budoux)

configure_package_config_file(
    cmake/budouxppConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/budouxppConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/budouxpp"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/budouxppConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMinorVersion
)

install(EXPORT budouxppTargets
    FILE budouxppTargets.cmake
    NAMESPACE budouxpp::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/budouxpp"
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/budouxppConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/budouxppConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/budouxpp"
)
