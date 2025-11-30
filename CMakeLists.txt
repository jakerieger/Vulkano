cmake_minimum_required(VERSION 3.14)
project(Vulkano)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

file(GLOB VULKANO_HEADERS Include/Vulkano/*.hpp Include/Vulkano/*.h)
file(GLOB VULKANO_SOURCES Source/*.cpp Source/*.c)

add_library(vulkano STATIC
    ${VULKANO_HEADERS}
    ${VULKANO_SOURCES}
)

target_include_directories(vulkano PRIVATE
    Include/Vulkano
)

# Testing application
add_subdirectory(Testbed)