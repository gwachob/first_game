cmake_minimum_required(VERSION 3.16)

project(first_game)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(first_game first_game.cc)
target_link_libraries(first_game ${CURSES_LIBRARIES})
