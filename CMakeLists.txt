cmake_minimum_required(VERSION 3.10)
project(game)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_subdirectory(libs/SDL EXCLUDE_FROM_ALL)

add_executable(game
    src/main.c
    src/utils.c
)

target_include_directories(game PRIVATE libs/HandmadeMath libs/stb)
target_link_libraries(game SDL3::SDL3 m)
