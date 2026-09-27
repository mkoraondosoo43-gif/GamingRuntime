cmake_minimum_required(VERSION 3.16)

project(GamingRuntime VERSION 2.5.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(gaming_runtime
    runtime/src/runtime.cpp
    runtime/src/storage.cpp
    runtime/src/bytecode.cpp
    runtime/src/asset_manager.cpp
    runtime/src/render.cpp
    runtime/src/input.cpp
    runtime/src/memory.cpp
    runtime/src/frame_scheduler.cpp
    runtime/src/audio.cpp
    runtime/src/display.cpp
    runtime/src/entity.cpp
    $<$<PLATFORM_ID:Android>:runtime/src/android_surface_display.cpp>
    $<$<PLATFORM_ID:Android>:runtime/src/android_input.cpp>
    $<$<PLATFORM_ID:Android>:runtime/src/android_runtime_host.cpp>
)

target_include_directories(gaming_runtime
    PUBLIC
        runtime/include
)

if(NOT ANDROID)
    add_executable(runtime_test
        runtime/tests/runtime_test.cpp
    )

    target_link_libraries(runtime_test
        PRIVATE
            gaming_runtime
    )

    add_executable(storage_test
        runtime/tests/storage_test.cpp
    )

    target_link_libraries(storage_test
        PRIVATE
            gaming_runtime
    )

    add_executable(entity_test
        runtime/tests/entity_test.cpp
    )

    target_link_libraries(entity_test
        PRIVATE
            gaming_runtime
    )

    enable_testing()
    add_test(NAME RuntimeCoreTest COMMAND runtime_test)
    add_test(NAME StorageTest COMMAND storage_test)
    add_test(NAME EntityTest COMMAND entity_test)
endif()

if(ANDROID)
    target_link_libraries(gaming_runtime PRIVATE android OpenSLES)

    add_library(gaming_runtime_android SHARED
        runtime/src/android_native_activity.cpp
    )

    target_include_directories(gaming_runtime_android
        PRIVATE
            runtime/include
    )

    target_link_libraries(gaming_runtime_android
        PRIVATE
            gaming_runtime
            android
            log
    )
endif()
