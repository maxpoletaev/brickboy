cmake_minimum_required(VERSION 3.10)

project(brickboy
        VERSION 1.0
        LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_CLANG_TIDY "clang-tidy")

execute_process(
        COMMAND date "+%Y.%m.%d"
        OUTPUT_VARIABLE BUILD_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE BUILD_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE)


add_compile_options(
        -Wall -Wextra -Wpedantic -Wconversion -Werror
        -Wno-gnu-zero-variadic-macro-arguments
        -Wno-sign-conversion
        -Wno-gnu-case-range)

#configure_file(config.h.in "${CMAKE_SOURCE_DIR}/src/config.h")
#add_custom_target(config DEPENDS src/config.h)

file(GLOB sources src/*.c src/*.h)
add_executable(brickboy ${sources})
#add_dependencies(brickboy config)

add_link_options(-fsanitize=address)
add_compile_options(-fsanitize=address)
add_executable(brickboy-asan ${sources})

