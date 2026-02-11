cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(rfl-firmware LANGUAGES C)

target_sources(app PRIVATE
    src/main.c
    src/at_client/at_client.c
)

target_include_directories(app PRIVATE include)