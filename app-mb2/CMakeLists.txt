cmake_minimum_required(VERSION 3.13.1)

set(BOARD "bbc_microbit_v2")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app-mb2 LANGUAGES C)

target_sources(app PRIVATE src/main.c)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
