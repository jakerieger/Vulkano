project(Vulkano)

find_package(Vulkan REQUIRED)

include(FetchContent)

# vk-bootstrap
FetchContent_Declare(
    vk-bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
    GIT_TAG v1.4.334
)
if (NOT vk-bootstrap_POPULATED AND NOT MSVC)
    FetchContent_Populate(vk-bootstrap)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
    add_subdirectory(${vk-bootstrap_SOURCE_DIR} ${vk-bootstrap_BINARY_DIR})
    string(REPLACE "-Wno-deprecated-declarations" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif ()

# VMA (Vulkan Memory Allocator)
FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    GIT_TAG v3.3.0
)

# Disable vk-bootstrap's strict warnings to avoid build errors
set(VK_BOOTSTRAP_WERROR OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build GLFW docs")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build GLFW tests")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build GLFW examples")
set(GLFW_INSTALL OFF CACHE BOOL "Generate GLFW installation target")

FetchContent_MakeAvailable(
    vk-bootstrap
    vma
    glfw
)
