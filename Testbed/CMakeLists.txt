project(Vulkano)

add_executable(Testbed
    main.cpp
)

target_link_libraries(Testbed PRIVATE vulkano glfw)

target_include_directories(Testbed PRIVATE ${CMAKE_SOURCE_DIR}/Include)