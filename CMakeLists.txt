cmake_minimum_required(VERSION 3.20)

project(brickboy
        VERSION 1.0
        LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_CLANG_TIDY "clang-tidy")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

find_package(raylib 4.2 REQUIRED)

add_compile_options(
    -Wall -Wextra -Wpedantic -Wimplicit-fallthrough -Wsign-conversion
    -Wno-gnu-zero-variadic-macro-arguments
    -Wno-gnu-case-range)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_link_options(-fsanitize=address)
    add_compile_options(-fsanitize=address)
endif()

file(GLOB sources src/*.c src/*.h)

# brickboy
add_executable(brickboy ${sources})
target_link_libraries(brickboy PRIVATE raylib)
