cmake_minimum_required(VERSION 3.25)

option(BUILD_TEST "Enable tests" ON)
option(BUILD_EXAMPLE "Build example executable" ON)
option(BUILD_BENCH "Build benchmark executable" OFF)

set(_budouxpp_vcpkg_manifest_features "")
if(BUILD_TEST)
    list(APPEND _budouxpp_vcpkg_manifest_features "tests")
endif()
if(BUILD_EXAMPLE)
    list(APPEND _budouxpp_vcpkg_manifest_features "example")
endif()
if(BUILD_BENCH)
    list(APPEND _budouxpp_vcpkg_manifest_features "benchmark")
endif()
set(VCPKG_MANIFEST_NO_DEFAULT_FEATURES ON CACHE BOOL
    "Resolve vcpkg dependencies only from CMake-selected manifest features" FORCE)
set(VCPKG_MANIFEST_FEATURES "${_budouxpp_vcpkg_manifest_features}" CACHE STRING
    "vcpkg manifest features required by the current CMake options" FORCE)
unset(_budouxpp_vcpkg_manifest_features)

project(budouxpp VERSION 0.1.0 LANGUAGES CXX)

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

if(BUILD_EXAMPLE)
    add_executable(example main.cpp)
    target_link_libraries(example PRIVATE budouxpp)
    target_compile_features(example PRIVATE cxx_std_20)
endif()

if(BUILD_TEST)
    enable_testing()
    add_subdirectory(test)
endif()

if(BUILD_BENCH)
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
