cmake_minimum_required(VERSION 3.20)

project(preview C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)
pkg_check_modules(HIDAPI REQUIRED IMPORTED_TARGET hidapi)

add_executable(preview main.c)

target_link_libraries(preview PRIVATE PkgConfig::SDL3 PkgConfig::HIDAPI)
