cmake_minimum_required(VERSION 3.13.4)
project(golc VERSION 1.0.0)

set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# Curses
set(
  CURSES_NEED_NCURSES TRUE
  CURSES_NEED_WIDE TRUE)
find_package(Curses REQUIRED)
message(STATUS "NCursesW found: ${CURSES_FOUND}")

add_definitions(-DNCURSES_WIDECHAR=1)
add_compile_definitions(NCURSES_WIDECHAR=1)

include_directories(
  ${CURSES_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(src)
