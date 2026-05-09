cmake_minimum_required(VERSION 3.10)
project(SOproject C)

set(CMAKE_C_STANDARD 99)


add_executable(SOproject main.c graph.c graph.h)


include_directories(${CMAKE_CURRENT_SOURCE_DIR})


target_link_libraries(SOproject ${CMAKE_CURRENT_SOURCE_DIR}/libraylib.a m pthread dl rt X11)