add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark PRIVATE budouxpp)
target_include_directories(benchmark PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_features(benchmark PRIVATE cxx_std_20)
