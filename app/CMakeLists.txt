cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app-sandbox LANGUAGES C)

target_sources(app PRIVATE src/main.c
                            src/srv/pwm.c)

target_include_directories(app PRIVATE include)