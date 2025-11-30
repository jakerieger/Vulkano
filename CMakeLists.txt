cmake_minimum_required(VERSION 3.14)
project(Vulkano)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Fetch 3rd party dependencies
include(CMake/FetchDeps.cmake)

file(GLOB VULKANO_HEADERS
    Include/Vulkano/**/*.h
    Include/Vulkano/**/*.hpp
    Include/Vulkano/*.h
    Include/Vulkano/*.hpp
)

file(GLOB VULKANO_SOURCES
    Source/**/.cpp
    Source/**/.c
    Source/*.cpp
    Source/*.c
)

add_library(vulkano STATIC
    ${VULKANO_HEADERS}
    ${VULKANO_SOURCES}
)

target_link_libraries(vulkano
    PUBLIC
    Vulkan::Vulkan
    vk-bootstrap::vk-bootstrap
    GPUOpen::VulkanMemoryAllocator
)


target_include_directories(vulkano
    PUBLIC Include
    PRIVATE Include/Vulkano
)

# Compiler warnings for vulkano itself
target_compile_options(vulkano PRIVATE
    $<$<CXX_COMPILER_ID:Clang,GNU>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Testing application
add_subdirectory(Testbed)