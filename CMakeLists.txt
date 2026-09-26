cmake_minimum_required(VERSION 3.16)

project(GamingRuntime VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(gaming_runtime
    runtime/src/runtime.cpp
)

target_include_directories(gaming_runtime
    PUBLIC
        runtime/include
)

add_executable(runtime_test
    runtime/tests/runtime_test.cpp
)

target_link_libraries(runtime_test
    PRIVATE
        gaming_runtime
)

enable_testing()
add_test(NAME RuntimeCoreTest COMMAND runtime_test)
