cmake_minimum_required(VERSION 3.13.1)

set(BOARD "stm32_min_dev")

if(NOT DEFINED ENV{ZEPHYR_BASE})
  zephyr_base_from_west_topdir()
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app-cdc LANGUAGES C)

target_sources(app PRIVATE src/main.c)

target_include_directories(app PRIVATE include)