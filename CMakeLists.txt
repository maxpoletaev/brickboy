cmake_minimum_required(VERSION 3.10)

project(brickboy
    VERSION 1.0
    LANGUAGES C)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_CLANG_TIDY "clang-tidy")

file(GLOB sources src/*.c src/*.h)
add_executable(brickboy ${sources})
add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Werror)
