cmake_minimum_required(VERSION 3.0)
project(Overmix)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(src)
add_subdirectory(interface/gui/viewer)
add_subdirectory(interface)
#add_subdirectory(benchmark)
add_subdirectory(trainer)